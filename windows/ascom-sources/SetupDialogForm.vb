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

Public Class SetupForm

    Private WithEvents myTimer As New System.Windows.Forms.Timer()
    Private CalCounter As Integer = 0
    Private isCalibrating As Boolean = False
    Private doneCalibrating As Boolean = False
    Private TimeSinceWake As Integer = 0

    Public MyDome As Dome

    Private Sub TimerEventProcessor(myObject As Object,
                                           ByVal myEventArgs As EventArgs) _
                                       Handles myTimer.Tick


        Dim ti As Integer
        'Beep()

        'BatteryVoltsDisplay.Text = Convert.ToString(Dome.BatteryVoltage)
        'ShutterVoltsDisplay.Text = Convert.ToString(Dome.ShutterVoltage)
        If Dome.isAtHome Then
            CalibrateButton.Enabled = True
            'StatusLine.Text = "At Home"
        Else
            CalibrateButton.Enabled = False
        End If
        BaseVolts.Text = Convert.ToString(Dome.BatteryVoltage / 100)
        ShutterVolts.Text = Convert.ToString(Dome.ShutterVoltage / 100)
        'LowVoltage.Text = Convert.ToString(Dome.LowVoltageCutoff / 100)
        BaseFirmware.Text = Dome.DomeFirmwareVersion
        ShutterFirmware.Text = Dome.ShutterFirmwareVersion

        'If doneCalibrating Then
        'DomeAzimuth.Text = Convert.ToString(Dome.StepsPerDomeTurn)
        'Else
        'DomeAzimuth.Text = Convert.ToString(Dome.CurrentAzimuth)
        'End If
        If Dome.isHoming Then
            StatusLine.Text = "Finding Home"
        Else
            If Dome.isCalibrating Then
                StatusLine.Text = "Calibrating... " & CalCounter.ToString()
                CalCounter = CalCounter + 1
            Else
                If Dome.isAtHome Then
                    StatusLine.Text = "At Home"
                    If isCalibrating Then
                        isCalibrating = False
                        doneCalibrating = True
                        HomeButton.Text = "Home"
                        'AzimuthBox.Text = "Calibration"
                    End If
                Else
                    If Dome.isAtPark Then
                        StatusLine.Text = "At Park"
                    End If
                    StatusLine.Text = ""
                End If
            End If
        End If

        If Dome.isAtHome Then
            CalibrateButton.Enabled = True
            'StatusLine.Text = "At Home"
        Else
            CalibrateButton.Enabled = False
        End If
        TimeSinceWake = TimeSinceWake + 1


        ' This is important.  If the DomeCommand("x") is issued while the dome is
        ' calibrating or homing the calibration/homing will never end, it continues foreever.
        ' This will also throw an occasional unhandled exception and then all bets are off.
        If Not (Dome.isCalibrating Or Dome.isHoming) Then

            ' The sleep timer is in 1/1000 of a second
            ' if it more than half expired since the last wakeup
            ' send another wakeup
            ti = Dome.ShutterSleepTimer
            ti = ti / 1000
            ti = ti / 2

            If (TimeSinceWake > ti) Then
                Try
                    MyDome.DomeCommand("x")
                    TimeSinceWake = 0
                Catch ex As Exception
                    ' Handling the random unhandle exception.  
                    ' Should log it somewhere?
                End Try
            
            End If
        End If

    End Sub

    Private Sub SetupForm_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        InitUI()
    End Sub

    Private Sub OK_Button_Click(sender As Object, e As EventArgs) Handles OK_Button.Click
        ' Persist new values of user settings to the ASCOM profile
        Dim ha As Double

        ha = Convert.ToDouble(HomePosition.Text)
        MyDome.SetHomePosition(ha)

        ha = Convert.ToDouble(ParkAzimuth.Text)
        MyDome.SetParkPosition(ha)

        ha = Convert.ToDouble(LowVoltage.Text)
        ha = ha * 100
        'If (ha <> MyDome.LowVoltageCutoff) Then
        MyDome.SetLowCutoff(ha)

        ha = Convert.ToDouble(SleepTimer.Text)
        ha = ha * 1000
        MyDome.SetSleepTimer(ha)

        'End If

        If ReverseCheckbox.Checked Then
            MyDome.SetReversed(1)
        Else
            MyDome.SetReversed(0)
        End If

        myTimer.Stop()
        Me.DialogResult = System.Windows.Forms.DialogResult.OK
        Me.Close()

    End Sub

    Private Sub InitUI()
        Dim lv As Double
        ParkAzimuth.Text = Dome.ParkPosition.ToString()
        HomePosition.Text = Dome.HomePosition.ToString()
        lv = Dome.LowVoltageCutoff / 100
        LowVoltage.Text = lv.ToString()
        BaseFirmware.Text = Dome.DomeFirmwareVersion
        ShutterFirmware.Text = Dome.ShutterFirmwareVersion
        lv = Dome.ShutterSleepTimer
        lv = lv / 1000
        SleepTimer.Text = lv.ToString()
        If Dome.IsReversed = 1 Then
            ReverseCheckbox.Checked = True
        Else
            ReverseCheckbox.Checked = False
        End If
        isCalibrating = False
        MyDome.DomeCommand("x")
        TimeSinceWake = 0
        myTimer.Interval = 1000
        myTimer.Start()

    End Sub

    Private Sub CalibrateButton_Click(sender As Object, e As EventArgs) Handles CalibrateButton.Click
        CalCounter = 0
        isCalibrating = True
        doneCalibrating = False
        'AzimuthBox.Text = "Dome Azimuth"
        HomeButton.Text = "Abort"

        ' do not sync on the home position while we are calibrating
        ReverseCheckbox.Checked = False
        Dome.SyncOnHome = False

        MyDome.CalibrateDome()
    End Sub

    Private Sub Cancel_Button_Click(sender As Object, e As EventArgs) Handles Cancel_Button.Click
        Me.DialogResult = System.Windows.Forms.DialogResult.Cancel
        myTimer.Stop()
        Me.Close()

    End Sub

    Private Sub HomeButton_Click(sender As Object, e As EventArgs) Handles HomeButton.Click
        doneCalibrating = False
        If isCalibrating Then
            isCalibrating = False
            doneCalibrating = False
            MyDome.AbortSlew()
            HomeButton.Text = "Home"
        Else
            MyDome.FindHome()
        End If
        isCalibrating = False
        'AzimuthBox.Text = "Dome Azimuth"
    End Sub

    Private Sub SyncCheckbox_CheckedChanged(sender As Object, e As EventArgs) Handles ReverseCheckbox.CheckedChanged
        If ReverseCheckbox.Checked = True Then
            Dome.SyncOnHome = True
        Else
            Dome.SyncOnHome = False
        End If
    End Sub

    Private Sub GroupBox2_Enter(sender As Object, e As EventArgs)

    End Sub

    Private Sub GroupBox1_Enter(sender As Object, e As EventArgs) Handles GroupBox1.Enter

    End Sub

    Private Sub BatteryBox_Enter(sender As Object, e As EventArgs) Handles BatteryBox.Enter

    End Sub

    Private Sub Label3_Click(sender As Object, e As EventArgs) Handles ShutterFirmware.Click

    End Sub
End Class
