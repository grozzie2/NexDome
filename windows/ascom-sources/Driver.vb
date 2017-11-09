'tabs=4
' --------------------------------------------------------------------------------
'
' ASCOM Dome driver for NexDome
'
' Description:	
'
' Implements:	ASCOM Dome interface version: 2.0
' Author:		  Gerry Rozema <gerryr@rozeware.com>
'
'    This package includes the drivers and sources for the NexDome
'    Copyright (C) 2016 Rozeware Development Ltd.

'    NexDome is free software: you can redistribute it and/or modify
'    it under the terms of the GNU General Public License as published by
'    the Free Software Foundation, either version 2 of the License, or
'    (at your option) any later version.
'
'    NexDome is distributed in the hope that it will be useful,
'    but WITHOUT ANY WARRANTY; without even the implied warranty of
'    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
'    GNU General Public License for more details.
'
'    You should have received a copy of the GNU General Public License
'    along with NexDome files.  If not, see <http://www.gnu.org/licenses/>.
'
' Edit Log:
'
' Date			Who	Vers	Description
' -----------	---	-----	-------------------------------------------------------
' 10-06-2016	gerry	0.1.0	Initial edit, from Dome template
' ---------------------------------------------------------------------------------
'
'
' Your driver's ID is ASCOM.NexDome.Dome
'
' The Guid attribute sets the CLSID for ASCOM.DeviceName.Dome
' The ClassInterface/None addribute prevents an empty interface called
' _Dome from being created and used as the [default] interface
'

' This definition is used to select code that's only applicable for one device type
#Const Device = "Dome"

Imports ASCOM
Imports ASCOM.Astrometry
Imports ASCOM.Astrometry.AstroUtils
Imports ASCOM.DeviceInterface
Imports ASCOM.Utilities

Imports System
Imports System.Collections
Imports System.Collections.Generic
Imports System.Globalization
Imports System.Runtime.InteropServices
Imports System.Text

<Guid("1fa53597-559f-4424-8044-1515c88f250b")>
<ClassInterface(ClassInterfaceType.None)>
Public Class Dome

    ' The Guid attribute sets the CLSID for ASCOM.NexDome.Dome
    ' The ClassInterface/None addribute prevents an empty interface called
    ' _NexDome from being created and used as the [default] interface

    ' TODO Replace the not implemented exceptions with code to implement the function or
    ' throw the appropriate ASCOM exception.
    '
    Implements IDomeV2

    '
    ' Driver ID and descriptive string that shows in the Chooser
    '
    Friend Shared driverID As String = "ASCOM.NexDome.Dome"
    Private Shared driverDescription As String = "NexDome"

    Friend Shared comPortProfileName As String = "COM Port" 'Constants used for Profile persistence
    ' Dont store home or park position in the profile, it's stored in the controller
    'Friend Shared ParkProfileName As String = "Park Position" 'Constants used for Profile persistence
    'Friend Shared HomeProfileName As String = "Home Position" 'Constants used for Profile persistence
    ' for now, we will let the driver sync on the home position
    ' this is scheduled to move into the dome firmware, but it's not there yet
    'Friend Shared SyncHomeProfileName As String = "Sync on Home"
    Friend Shared traceStateProfileName As String = "Trace Level"
    Friend Shared comPortDefault As String = "COM1"
    Friend Shared traceStateDefault As String = "False"

    Friend Shared comPort As String ' Variables to hold the currrent device configuration
    Friend Shared traceState As Boolean
    Friend Shared SerialPort As ASCOM.Utilities.Serial

    Friend Shared ControllerVersion As String ' the firmware version reported by the dome
    Friend Shared ControllerType As Integer ' 0 = unknown  1=rotation controller 2=shutter controller
    Friend Shared CurrentAzimuth As Double = 0  ' where the dome is pointed now
    Friend Shared DomeShutterState As ShutterState = ShutterState.shutterError
    Friend Shared ShutterAlt As Double = 0
    Friend Shared isAtHome As Boolean = False
    Friend Shared SyncOnHome = False
    Friend Shared isInMotion As Boolean = False
    Friend Shared isAtPark As Boolean = False
    Friend Shared BatteryVoltage As Double = 0
    Friend Shared ShutterVoltage As Double = 0
    Friend Shared ShutterSleepTimer As Integer = 0
    Friend Shared LowVoltageCutoff As Double = 0
    Friend Shared IsReversed As Integer = -1

    Private TicksLastCommand As Integer = 0
    Private isSlewing As Boolean = False
    Private isParking As Boolean = False
    ' set these up so we dont wait on a status change at startup
    Private TimeSinceClose As Integer = 60
    Private TimeSinceOpen As Integer = 60
    Friend Shared isHoming As Boolean = False
    Friend Shared isCalibrating As Boolean = False
    Friend Shared StepsPerDomeTurn As Integer = 0

    '  bit of a kludge, but, if I make these shared friends, then the setup dialog can access them
    Friend Shared ParkPosition As Double = 0
    Friend Shared HomePosition As Double = 0
    Friend Shared DomeFirmwareVersion As String = ""
    Friend Shared ShutterFirmwareVersion As String = "Not Connected"
    Friend Shared FoundShutter As Boolean = "False"


    Public Shared connectedState As Boolean ' Private variable to hold the connected state
    Private utilities As Util ' Private variable to hold an ASCOM Utilities object
    Private astroUtilities As AstroUtils ' Private variable to hold an AstroUtils object to provide the Range method
    Private TL As TraceLogger ' Private variable to hold the trace logger object (creates a diagnostic log file with information that you specify)

    '
    ' Constructor - Must be public for COM registration!
    '
    Public Sub New()

        ReadProfile() ' Read device configuration from the ASCOM Profile store
        TL = New TraceLogger("", "NexDome")
        TL.Enabled = traceState
        TL.LogMessage("Dome", "Starting initialisation")

        'connectedState = False ' Initialise connected to false
        utilities = New Util() ' Initialise util object
        astroUtilities = New AstroUtils 'Initialise new astro utiliites object

        'TODO: Implement your additional construction here

        TL.LogMessage("Dome", "Completed initialisation")
    End Sub

    '
    ' PUBLIC COM INTERFACE IDomeV2 IMPLEMENTATION
    '

#Region "Common properties and methods"
    ''' <summary>
    ''' Displays the Setup Dialog form.
    ''' If the user clicks the OK button to dismiss the form, then
    ''' the new settings are saved, otherwise the old values are reloaded.
    ''' THIS IS THE ONLY PLACE WHERE SHOWING USER INTERFACE IS ALLOWED!
    ''' </summary>
    Public Sub SetupDialog() Implements IDomeV2.SetupDialog
        ' consider only showing the setup dialog if not connected
        ' or call a different dialog if connected
        If IsConnected Then
            Using F As SetupForm = New SetupForm()
                F.MyDome = Me
                Dim result As System.Windows.Forms.DialogResult = F.ShowDialog()
                If result = DialogResult.OK Then
                    SetHomePosition(HomePosition)
                    WriteProfile() ' Persist device configuration values to the ASCOM Profile store
                End If
            End Using
        Else
            Using F As ComPortChooserForm = New ComPortChooserForm()
                Dim result As System.Windows.Forms.DialogResult = F.ShowDialog()
                If result = DialogResult.OK Then
                    WriteProfile() ' Persist device configuration values to the ASCOM Profile store
                End If
            End Using
        End If

    End Sub

    Public ReadOnly Property SupportedActions() As ArrayList Implements IDomeV2.SupportedActions
        Get
            TL.LogMessage("SupportedActions Get", "Returning empty arraylist")
            Return New ArrayList()
        End Get
    End Property

    Public Function Action(ByVal ActionName As String, ByVal ActionParameters As String) As String Implements IDomeV2.Action
        Throw New ActionNotImplementedException("Action " & ActionName & " is not supported by this driver")
    End Function

    Public Sub CommandBlind(ByVal Command As String, Optional ByVal Raw As Boolean = False) Implements IDomeV2.CommandBlind
        CheckConnected("CommandBlind")
        ' Call CommandString and return as soon as it finishes
        Me.CommandString(Command, Raw)
        ' or
        Throw New MethodNotImplementedException("CommandBlind")
    End Sub

    Public Function CommandBool(ByVal Command As String, Optional ByVal Raw As Boolean = False) As Boolean _
        Implements IDomeV2.CommandBool
        CheckConnected("CommandBool")
        Dim ret As String = CommandString(Command, Raw)
        ' TODO decode the return string and return true or false
        ' or
        Throw New MethodNotImplementedException("CommandBool")
    End Function

    Private commandLocker As object = New object
    Public Function CommandString(ByVal Command As String, Optional ByVal Raw As Boolean = False) As String _
        Implements IDomeV2.CommandString
        'CheckConnected("CommandString")

        SyncLock commandLocker
            ' it's a good idea to put all the low level communication with the device here,
            ' then all communication calls this function
            ' you need something to ensure that only one command is in progress at a time
            Dim TxString, RxString, Result As String
            ' start by flushing any leftovers in serial buffers
            SerialPort.ClearBuffers()
            TxString = Command & Chr(10)
            ' send a command
            SerialPort.Transmit(TxString)
            ' get the response first char which tells us what is being reported
            Result = SerialPort.ReceiveCounted(1)
            ' now get the rest of what came in the response
            RxString = SerialPort.ReceiveTerminated(Chr(10))

            TL.LogMessage("CommandString", Result & RxString)


            If Result = "A" Then
                ' this was an abort command
                ' reset timers so we dont sit on a bad display for opening/closing states
                TimeSinceClose = 60
                TimeSinceOpen = 60
                Return ""
            End If
            If Result = "G" Then
                ' this was a goto, results will come from azimuth and slewing checks
                Return ""
            End If
            If Result = "Q" Then
                ' this was a query for current azimuth
                CurrentAzimuth = Convert.ToDouble(RxString)
            End If
            If Result = "U" Then
                Dim s As Integer
                ' this was a query for current shutter status
                s = Convert.ToInt16(RxString)
                TimeSinceClose = TimeSinceClose + 1
                TimeSinceOpen = TimeSinceOpen + 1
                Select Case s
                    Case 1
                        ' When we get an open or close command, shutter may not pick it up for
                        ' 30 seconds, so, lets keep the state for 30 seconds after we send the
                        ' command, makes the displays etc look better
                        ' improvements in the firmware now mean we only need 5 read
                        ' to be correct
                        If TimeSinceClose > 5 Then
                            DomeShutterState = ShutterState.shutterOpen
                        End If
                    Case 2
                        DomeShutterState = ShutterState.shutterOpening
                    Case 3
                        ' keep the state for 5 seconds after the open/close command
                        ' to allow wireless links to catch up
                        'TimeSinceOpen = TimeSinceOpen + 1
                        If TimeSinceOpen > 5 Then
                            DomeShutterState = ShutterState.shutterClosed
                        End If
                    Case 4
                        DomeShutterState = ShutterState.shutterClosing
                    Case Else
                        DomeShutterState = ShutterState.shutterError
                End Select

            End If
            If Result = "B" Then
                ' this was a request for the shutter altitude
                ShutterAlt = Convert.ToDouble(RxString)
            End If
            If Result = "Z" Then
                Dim res As Int16
                ' this was a request for home position sensor status
                res = Convert.ToInt16(RxString)
                If (res = 0) Then
                    isAtHome = False
                Else
                    isAtHome = True
                End If
            End If
            If Result = "M" Then
                Dim m As Int16
                ' is the dome in motion ?
                m = Convert.ToInt16(RxString)
                If m = 0 Then
                    isInMotion = False
                Else
                    isInMotion = True
                End If
            End If
            If (Result = "K") Then
                Dim btest() As String = Split(RxString)
                BatteryVoltage = Convert.ToDouble(btest(1))
                ShutterVoltage = Convert.ToDouble(btest(2))
                LowVoltageCutoff = Convert.ToDouble(btest(3))
            End If

            If Result = "V" Then
                Dim vtest() As String = Split(RxString)

                ' we are parsing out the version return string
                ControllerType = 0

                If vtest(0) = "NexDome" Then
                    ' this is a rotation controller
                    ControllerType = 1
                    DomeFirmwareVersion = vtest(2)
                    If vtest(5) IsNot "" Then
                        ShutterFirmwareVersion = vtest(5)
                        FoundShutter = True
                    Else
                        FoundShutter = False
                        ShutterFirmwareVersion = "Not Connected"

                    End If
                Else
                    If vtest(0) = "NexShutter" Then
                        ' we can get the shutter version from the rotator
                        ' too
                        ControllerType = 2
                        ShutterFirmwareVersion = vtest(2)
                    End If
                End If
                ControllerVersion = vtest(3)
            End If

            If Result = "T" Then
                StepsPerDomeTurn = Convert.ToInt64(RxString)
            End If
            If Result = "I" Then
                HomePosition = Convert.ToDouble(RxString)
            End If
            If Result = "N" Then
                ParkPosition = Convert.ToDouble(RxString)
            End If
            If Result = "Y" Then
                IsReversed = Convert.ToInt16(RxString)
            End If
            If Result = "R" Then
                ShutterSleepTimer = Convert.ToInt32(RxString)
            End If

            Return ""
        End SyncLock

    End Function

    Public Property Connected() As Boolean Implements IDomeV2.Connected
        Get
            TL.LogMessage("Connected Get", IsConnected.ToString())
            Return IsConnected
        End Get
        Set(value As Boolean)
            TL.LogMessage("Connected Set", value.ToString())
            If value = IsConnected Then
                Return
            End If

            If value Then
                ReadProfile()
                connectedState = False
                TL.LogMessage("Connected Set", "Connecting to port " + comPort)
                ' TODO connect to the device
                SerialPort = New ASCOM.Utilities.Serial
                SerialPort.PortName = comPort
                SerialPort.Speed = ASCOM.Utilities.SerialSpeed.ps9600
                SerialPort.DataBits = 8
                SerialPort.Parity = IO.Ports.Parity.None
                SerialPort.StopBits = IO.Ports.StopBits.One
                SerialPort.DTREnable = True
                SerialPort.Handshake = IO.Ports.Handshake.None
                ' It is important to set this timeout
                ' we expect our hardware to respond quickly on requests
                ' and we dont want threads hung up in long timeouts when
                ' things are not plugged in and responding
                SerialPort.ReceiveTimeoutMs = 500

                Try
                    ' now open the com port
                    SerialPort.Connected = True
                    connectedState = True
                    ' clear any garbage from the buffers that may be there
                    SerialPort.ClearBuffers()
                Catch ex As Exception
                    connectedState = False
                End Try

                ' if we got to here, and the state is true
                ' we successfully opened the com port
                If connectedState = True Then
                    ' now we have to see if our dome is present on the other end of the connection
                    ' preset our version string to an empty string
                    ControllerVersion = ""


                    ' now ask the dome what is the firmware version
                    Try
                        CommandString("v")
                    Catch ex As Exception
                        ' if we get here, then there was an error communicating with the dome
                        ' so disconnect from the com port
                        connectedState = False
                    End Try
                End If

                ' if we get here, and connected is still true
                ' then we had a response on the serial port
                If ControllerType = 0 Then
                    ' we didn't get a recognizeable response to a version request
                    ' likely it's not our dome connected on this serial port
                    TL.LogMessage("Connect", "No response From dome for version request")
                    connectedState = False
                Else
                    connectedState = True
                End If

                If connectedState = False Then
                    ' if we get here, we didn't get a proper handshake from the dome controller
                    ' so we should clean up a bit
                    SerialPort.Connected = False
                    SerialPort.Dispose()
                    SerialPort = Nothing
                Else
                    ' If we get here, then we did get connected, and found our controller
                    ' We should flesh out a few details right away
                    ' Get the home position
                    DomeCommand("i")
                    ' Get the park position
                    DomeCommand("n")
                    ' Get battery status
                    DomeCommand("k")
                    ' Get current position
                    DomeCommand("q")
                    ' Get the sleep timer
                    DomeCommand("r")
                End If


            Else
                connectedState = False
                TL.LogMessage("Connected Set", "Disconnecting from port " + comPort)
                ' TODO disconnect from the device
                SerialPort.Connected = False
                SerialPort.Dispose()
                SerialPort = Nothing

            End If
        End Set
    End Property

    Public ReadOnly Property Description As String Implements IDomeV2.Description
        Get
            ' this pattern seems to be needed to allow a public property to return a private field
            Dim d As String = driverDescription
            TL.LogMessage("Description Get", d)
            Return d
        End Get
    End Property

    Public ReadOnly Property DriverInfo As String Implements IDomeV2.DriverInfo
        Get
            Dim m_version As Version = System.Reflection.Assembly.GetExecutingAssembly().GetName().Version
            ' TODO customise this driver description
            Dim s_driverInfo As String = "NexDome Version: " + m_version.Major.ToString() + "." + m_version.Minor.ToString()
            TL.LogMessage("DriverInfo Get", s_driverInfo)
            Return s_driverInfo
        End Get
    End Property

    Public ReadOnly Property DriverVersion() As String Implements IDomeV2.DriverVersion
        Get
            ' Get our own assembly and report its version number
            TL.LogMessage("DriverVersion Get", Reflection.Assembly.GetExecutingAssembly.GetName.Version.ToString(2))
            Return Reflection.Assembly.GetExecutingAssembly.GetName.Version.ToString(2)
        End Get
    End Property

    Public ReadOnly Property InterfaceVersion() As Short Implements IDomeV2.InterfaceVersion
        Get
            TL.LogMessage("InterfaceVersion Get", "2")
            Return 2
        End Get
    End Property

    Public ReadOnly Property Name As String Implements IDomeV2.Name
        Get
            Dim s_name As String = "NexDome"
            TL.LogMessage("Name Get", s_name)
            Return s_name
        End Get
    End Property

    Public Sub Dispose() Implements IDomeV2.Dispose
        ' Clean up the tracelogger and util objects
        TL.Enabled = False
        TL.Dispose()
        TL = Nothing
        utilities.Dispose()
        utilities = Nothing
        astroUtilities.Dispose()
        astroUtilities = Nothing
    End Sub

#End Region

#Region "IDome Implementation"

    Public Sub AbortSlew() Implements IDomeV2.AbortSlew
        ' This is a mandatory parameter but we have no action to take in this simple driver
        Dim s As String
        TL.LogMessage("AbortSlew", "Completed")
        ' just in case this goes thru in mid command
        ' the embedded cr will force the controller to start processing a new line
        ' so it'll catch the second abort if not the first
        s = "a" & Chr(10) & "a"
        ' send this one now
        DomeCommand("a")
        isCalibrating = False

    End Sub

    Public ReadOnly Property Altitude() As Double Implements IDomeV2.Altitude
        Get
            TL.LogMessage("Altitude Get", Convert.ToString(ShutterAlt))
            ReadDomeStatus()
            Return ShutterAlt
            'Throw New ASCOM.PropertyNotImplementedException("Altitude", False)
        End Get
    End Property

    Public ReadOnly Property AtHome() As Boolean Implements IDomeV2.AtHome
        Get
            TL.LogMessage("AtHome", isAtHome.ToString())
            ReadDomeStatus()
            Return isAtHome
            'Throw New ASCOM.PropertyNotImplementedException("AtHome", False)
        End Get
    End Property

    Public ReadOnly Property AtPark() As Boolean Implements IDomeV2.AtPark
        Get
            TL.LogMessage("AtPark", isAtPark.ToString())
            ReadDomeStatus()
            Return isAtPark
            'Throw New ASCOM.PropertyNotImplementedException("AtPark", False)
        End Get
    End Property

    Public ReadOnly Property Azimuth() As Double Implements IDomeV2.Azimuth
        Get
            TL.LogMessage("Azimuth", CurrentAzimuth.ToString())
            ReadDomeStatus()
            Return CurrentAzimuth
            'Throw New ASCOM.PropertyNotImplementedException("Azimuth", False)
        End Get
    End Property

    Public ReadOnly Property CanFindHome() As Boolean Implements IDomeV2.CanFindHome
        Get
            TL.LogMessage("CanFindHome Get", True.ToString())
            Return True
        End Get
    End Property

    Public ReadOnly Property CanPark() As Boolean Implements IDomeV2.CanPark
        Get
            TL.LogMessage("CanPark Get", True.ToString())
            Return True
        End Get
    End Property

    Public ReadOnly Property CanSetAltitude() As Boolean Implements IDomeV2.CanSetAltitude
        Get
            TL.LogMessage("CanSetAltitude Get", False.ToString())
            Return False
            'TL.LogMessage("CanSetAltitude Get", True.ToString())
            'Return True
        End Get
    End Property

    Public ReadOnly Property CanSetAzimuth() As Boolean Implements IDomeV2.CanSetAzimuth
        Get
            TL.LogMessage("CanSetAzimuth Get", True.ToString())
            Return True
        End Get
    End Property

    Public ReadOnly Property CanSetPark() As Boolean Implements IDomeV2.CanSetPark
        Get
            TL.LogMessage("CanSetPark Get", True.ToString())
            Return True
        End Get
    End Property

    Public ReadOnly Property CanSetShutter() As Boolean Implements IDomeV2.CanSetShutter
        Get
            TL.LogMessage("CanSetShutter Get", True.ToString())
            Return True
        End Get
    End Property

    Public ReadOnly Property CanSlave() As Boolean Implements IDomeV2.CanSlave
        Get
            TL.LogMessage("CanSlave Get", False.ToString())
            Return False
        End Get
    End Property

    Public ReadOnly Property CanSyncAzimuth() As Boolean Implements IDomeV2.CanSyncAzimuth
        Get
            TL.LogMessage("CanSyncAzimuth Get", True.ToString())
            Return True
        End Get
    End Property

    Public Sub CloseShutter() Implements IDomeV2.CloseShutter
        TL.LogMessage("CloseShutter", " ")
        MotionStarting()
        DomeCommand("e")
        DomeShutterState = ShutterState.shutterClosing
        TimeSinceClose = 0
    End Sub

    Public Sub FindHome() Implements IDomeV2.FindHome
        TL.LogMessage("FindHome", "starting")
        MotionStarting()
        DomeCommand("h")
        isHoming = True
        'Throw New ASCOM.MethodNotImplementedException("FindHome")
    End Sub

    Public Sub OpenShutter() Implements IDomeV2.OpenShutter
        TL.LogMessage("OpenShutter", "entry")
        Select Case DomeShutterState
            Case ShutterState.shutterOpen
                TL.LogMessage("OpenShutter", "already open")
            Case ShutterState.shutterClosed
                TL.LogMessage("OpenShutter", "Opening")
                DomeCommand("d")
                DomeShutterState = ShutterState.shutterOpening
                TimeSinceOpen = 0
            Case ShutterState.shutterOpening
                TL.LogMessage("OpenShutter", "Already in Progress")
            Case ShutterState.shutterClosing
                TL.LogMessage("OpenShutter", "Cannot open while shutter is closing")
            Case Else
                ' in this case, the rotator is not in contact with a
                ' shutter controller
                TL.LogMessage("OpenShutter", "Shutter error")
                Throw New ASCOM.InvalidOperationException

        End Select
    End Sub

    Public Sub Park() Implements IDomeV2.Park
        Dim az As Double
        az = Math.Round(ParkPosition, 1)
        TL.LogMessage("Park", az.ToString())
        'CloseShutter()
        SlewToAzimuth(az)
        MotionStarting()
        isParking = True
    End Sub

    Public Sub SetPark() Implements IDomeV2.SetPark
        Dim aa As Double
        aa = Math.Round(Azimuth, 1)
        TL.LogMessage("SetPark", aa.ToString())
        SetParkPosition(aa)

    End Sub

    Public ReadOnly Property ShutterStatus() As ShutterState Implements IDomeV2.ShutterStatus
        Get
            TL.LogMessage("ShutterStatus", DomeShutterState.ToString())
            ReadDomeStatus()
            Return DomeShutterState
        End Get
    End Property

    Public Property Slaved() As Boolean Implements IDomeV2.Slaved
        Get
            TL.LogMessage("Slaved Get", False.ToString())
            Return False
        End Get
        Set(value As Boolean)
            TL.LogMessage("Slaved Set", "not implemented")
            Throw New ASCOM.PropertyNotImplementedException("Slaved", True)
        End Set
    End Property

    Public Sub SlewToAltitude(Altitude As Double) Implements IDomeV2.SlewToAltitude
        Dim TxString As String
        Dim alt As Double
        Throw New ASCOM.MethodNotImplementedException("SlewToAltitude")
        'TL.LogMessage("SlewToAltitude", Convert.ToString(Altitude))
        'alt = Math.Round(Altitude, 1)
        ''alt = 90 - alt
        'If alt > 90 Then
        ' Throw New ASCOM.InvalidValueException
        'End If
        'If alt < 0 Then
        'Throw New ASCOM.InvalidValueException
        'End If
        'TxString = "f " & alt.ToString()
        'MotionStarting()
        'DomeCommand(TxString)
        Return
    End Sub

    Public Sub SlewToAzimuth(Azimuth As Double) Implements IDomeV2.SlewToAzimuth
        Dim TxString As String
        Dim az As Double
        'TL.LogMessage("SlewToAzimuth", Azimuth.ToString())
        az = Math.Round(Azimuth, 1)
        If az > 360 Then
            Throw New ASCOM.InvalidValueException
        End If
        If az < 0 Then
            Throw New ASCOM.InvalidValueException
        End If
        TxString = "g " & az.ToString()
        MotionStarting()
        DomeCommand(TxString)
        Return
    End Sub

    Public ReadOnly Property Slewing() As Boolean Implements IDomeV2.Slewing
        Get
            ReadDomeStatus()
            TL.LogMessage("Slewing Get", Slewing.ToString())
            Return isSlewing
        End Get
    End Property

    Public Sub SyncToAzimuth(Azimuth As Double) Implements IDomeV2.SyncToAzimuth
        Dim TxString As String
        Dim az As Double
        TL.LogMessage("SyncToAzimuth", Azimuth.ToString())
        az = Math.Round(Azimuth, 1)
        If az < 0 Then
            Throw New ASCOM.InvalidValueException
        End If
        If az > 360 Then
            Throw New ASCOM.InvalidValueException
        End If
        TxString = "s " & az.ToString()
        DomeCommand(TxString)
        ' conform complains if we dont report the new azimuth right away
        ' so ask the dome where it's pointed now
        DomeCommand("q")
        Return
    End Sub

#End Region

#Region "Private properties and methods"
    ' here are some useful properties and methods that can be used as required
    ' to help with
#Region "Nexdome hardware communications"

    ''' <summary>
    ''' send a command to the dome controller, marking our timestamp for further command pacing
    ''' </summary>
    Public Sub DomeCommand(ByVal Command As String)
        ' timestamp our last command to help with command pacing
        TicksLastCommand = My.Computer.Clock.TickCount
        CommandString(Command)
    End Sub

    ''' <summary>
    ''' All methods that move the dome will call this function to reset state variables
    ''' </summary>
    Private Sub MotionStarting()
        isSlewing = True
        isAtPark = False
        isAtHome = False
        isParking = False   ' park function sets this correctly after calling here
        isHoming = False
        isCalibrating = False

        Return
    End Sub

    Friend Sub SetLowCutoff(Cutoff As Integer)
        Dim TxString As String
        TxString = "k " & Cutoff.ToString()
        DomeCommand(TxString)

    End Sub
    ''' <summary>
    ''' Read the entire status of the dome
    ''' but since some of the methods/properties are called fast and furious so pace
    ''' the serial port, only query the physical hardware at a maximum rate of
    ''' once a second
    ''' </summary>
    Private Sub ReadDomeStatus()
        Dim Ticksnow As Integer
        Dim delta As Integer

        TL.LogMessage("ReadDomeStatus", "Entry")
        Ticksnow = My.Computer.Clock.TickCount
        Try
            delta = Ticksnow - TicksLastCommand
        Catch ex As System.OverflowException
            TicksLastCommand = Ticksnow
            delta = 1000
        End Try
        If delta < 1000 Then
            ' we have done this less than a second ago
            ' so just return with state variables unchanged
            TL.LogMessage("ReadDomeStatus", "To fast, ignore")
            Return
        End If
        ' Timestamp our last check
        TicksLastCommand = Ticksnow
        CheckConnected("ReadDomeStatus")

        ' first we get current azimuth
        DomeCommand("q")
        ' Next we ask the dome if it is in motion
        DomeCommand("m")
        If isInMotion Then
            'the dome is turning
            isSlewing = True

            ' There is no point in doing shutter checks if the dome is turning
            ' because the rotation controller doesn't talk to the shutter controller
            ' while the motor is running
            Return
        End If

        ' The dome is not turning, so lets check our shutter
        DomeCommand("u")    ' get shutter movement status
        DomeCommand("b")    ' get shutter position
        If DomeShutterState = ShutterState.shutterClosing Or DomeShutterState = ShutterState.shutterOpening Then
            ' The shutter is in motion
            isSlewing = True
            Return
        End If


        If DomeShutterState = ShutterState.shutterOpen Then
            If ShutterAlt < 90.0 Then
                ' The only way this can happen, if a movement was started, then stopped
                ' with an abort, so shutter is not open or closed, but somewhere in between
                ' Since we are now advertising an all or nothing shutter to ascom, we have to 
                ' report this as an unknown state
                DomeShutterState = ShutterState.shutterError
            End If
        End If

        ' see if a shutter has talked to us
        If DomeShutterState = ShutterState.shutterError Then
            If FoundShutter = True Then
                ShutterFirmwareVersion = ""
                FoundShutter = False
                DomeCommand("v")
            End If
        Else
            ' and we dont have a firmware version for it
            If FoundShutter = False Then
                ' lets get firmware versions
                DomeCommand("v")
            End If

        End If
        ' neither the dome or shutter are in motions
        ' so slewing now becomes false
        isSlewing = False
        If isParking Then
            If CurrentAzimuth = ParkPosition Then
                isAtPark = True
                isParking = False
            End If
        End If

        ' if we get this far, the dome is not turning, and the shutter is not moving
        DomeCommand("z")    ' check if the home sensor is active
        If isAtHome Then
            isHoming = False
            If isCalibrating Then
                isCalibrating = False
            End If
            If SyncOnHome Then
                SyncToAzimuth(HomePosition)
            End If
        End If
        DomeCommand("k")    ' Read power status
        'DomeCommand("t")    ' ask how many steps it takes for a full revolution
        If HomePosition = 0 Then
            ' either the home position hasn't been set, or, we just started up and have not read it yet
            ' in either case, ask the controller where home should be
            DomeCommand("i")
        End If
        If ParkPosition = 0 Then
            ' either the park position hasn't been set, or, we just started up and have not read it yet
            ' in either case, ask the controller where park should be
            DomeCommand("n")
        End If
        If IsReversed = -1 Then
            ' if we have not read the reversed flag, read it now
            DomeCommand("y")
        End If
        ' read sleep timer
        DomeCommand("r")

    End Sub

    Friend Sub SetSleepTimer(r As Integer)
        Dim TxString As String
        TxString = "r " & r.ToString()
        DomeCommand(TxString)
    End Sub

    Friend Sub SetReversed(r As Integer)
        Dim TxString As String
        TL.LogMessage("Set Reversed", r.ToString())
        If r > 1 Then
            Throw New ASCOM.InvalidValueException
        End If
        If r < 0 Then
            Throw New ASCOM.InvalidValueException
        End If
        TxString = "y " & r.ToString()
        DomeCommand(TxString)
        ' we have sent the command to set the home
        ' lets read the home position now, make sure it stuck
        ' dont have to do this, it comes back in the set response
        'DomeCommand("i")

    End Sub

    Friend Sub SetHomePosition(Azimuth As Double)
        Dim TxString As String
        Dim az As Double
        TL.LogMessage("Set Home Position", Azimuth.ToString())
        az = Math.Round(Azimuth, 1)
        If az < 0 Then
            Throw New ASCOM.InvalidValueException
        End If
        If az > 360 Then
            Throw New ASCOM.InvalidValueException
        End If
        TxString = "j " & az.ToString()
        DomeCommand(TxString)
        ' we have sent the command to set the home
        ' lets read the home position now, make sure it stuck
        ' dont have to do this, it comes back in the set response
        'DomeCommand("i")

    End Sub

    Friend Sub SetParkPosition(Azimuth As Double)
        Dim TxString As String
        Dim az As Double
        TL.LogMessage("Set Park Position", Azimuth.ToString())
        az = Math.Round(Azimuth, 1)
        If az < 0 Then
            Throw New ASCOM.InvalidValueException
        End If
        If az > 360 Then
            Throw New ASCOM.InvalidValueException
        End If
        TxString = "l " & az.ToString()
        DomeCommand(TxString)
        ' we have sent the command to set the park
        ' lets read the park position now, make sure it stuck
        ' dont actually have to do it, it comes back in the set response
        'DomeCommand("i")

    End Sub

    Friend Sub CalibrateDome()
        TL.LogMessage("NexDome Calibrate", "Start")
        DomeCommand("c")   ' send the calibrate
        MotionStarting()
        isCalibrating = True

    End Sub


#End Region

#Region "ASCOM Registration"

    Private Shared Sub RegUnregASCOM(ByVal bRegister As Boolean)

        Using P As New Profile() With {.DeviceType = "Dome"}
            If bRegister Then
                P.Register(driverID, driverDescription)
            Else
                P.Unregister(driverID)
            End If
        End Using

    End Sub

    <ComRegisterFunction()>
    Public Shared Sub RegisterASCOM(ByVal T As Type)

        RegUnregASCOM(True)

    End Sub

    <ComUnregisterFunction()>
    Public Shared Sub UnregisterASCOM(ByVal T As Type)

        RegUnregASCOM(False)

    End Sub

#End Region



    ''' <summary>
    ''' Returns true if there is a valid connection to the driver hardware
    ''' </summary>
    Private ReadOnly Property IsConnected As Boolean
        Get
            ' TODO check that the driver hardware connection exists and is connected to the hardware
            Return connectedState
        End Get
    End Property

    ''' <summary>
    ''' Use this function to throw an exception if we aren't connected to the hardware
    ''' </summary>
    ''' <param name="message"></param>
    Private Sub CheckConnected(ByVal message As String)
        If Not IsConnected Then
            Throw New NotConnectedException(message)
        End If
    End Sub


    ''' <summary>
    ''' Read the device configuration from the ASCOM Profile store
    ''' </summary>
    Friend Sub ReadProfile()
        Using driverProfile As New Profile()
            Dim res As String
            driverProfile.DeviceType = "Dome"
            traceState = Convert.ToBoolean(driverProfile.GetValue(driverID, traceStateProfileName, String.Empty, traceStateDefault))
            comPort = driverProfile.GetValue(driverID, comPortProfileName, String.Empty, comPortDefault)
            'res = driverProfile.GetValue(driverID, ParkProfileName, String.Empty, "0")
            'ParkPosition = Convert.ToDouble(res)
            'res = driverProfile.GetValue(driverID, SyncHomeProfileName, String.Empty, "False")
            'SyncOnHome = Convert.ToBoolean(res)
            ' Not storeing this in the profile because it's stored in the eeprom on the dome controller
            'res = driverProfile.GetValue(driverID, HomeProfileName, String.Empty, "0")
            'HomePosition = Convert.ToDouble(res)
        End Using
    End Sub

    ''' <summary>
    ''' Write the device configuration to the  ASCOM  Profile store
    ''' </summary>
    Friend Sub WriteProfile()
        Using driverProfile As New Profile()
            driverProfile.DeviceType = "Dome"
            driverProfile.WriteValue(driverID, traceStateProfileName, traceState.ToString())
            driverProfile.WriteValue(driverID, comPortProfileName, comPort.ToString())
            'driverProfile.WriteValue(driverID, ParkProfileName, ParkPosition.ToString())
            'driverProfile.WriteValue(driverID, SyncHomeProfileName, SyncOnHome.ToString())
            'driverProfile.WriteValue(driverID, HomeProfileName, HomePosition.ToString())
        End Using

    End Sub

#End Region

End Class
