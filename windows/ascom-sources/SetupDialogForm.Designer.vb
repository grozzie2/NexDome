<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()>
Partial Class SetupForm
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()>
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()>
    Private Sub InitializeComponent()
        Me.OK_Button = New System.Windows.Forms.Button()
        Me.CalibrateButton = New System.Windows.Forms.Button()
        Me.HomeBox = New System.Windows.Forms.GroupBox()
        Me.HomePosition = New System.Windows.Forms.TextBox()
        Me.ParkBox = New System.Windows.Forms.GroupBox()
        Me.GroupBox4 = New System.Windows.Forms.GroupBox()
        Me.TextBox1 = New System.Windows.Forms.TextBox()
        Me.ParkAzimuth = New System.Windows.Forms.TextBox()
        Me.Cancel_Button = New System.Windows.Forms.Button()
        Me.BatteryBox = New System.Windows.Forms.GroupBox()
        Me.ShutterVolts = New System.Windows.Forms.Label()
        Me.BaseVolts = New System.Windows.Forms.Label()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.Label1 = New System.Windows.Forms.Label()
        Me.HomeButton = New System.Windows.Forms.Button()
        Me.GroupBox1 = New System.Windows.Forms.GroupBox()
        Me.StatusLine = New System.Windows.Forms.Label()
        Me.ReverseCheckbox = New System.Windows.Forms.CheckBox()
        Me.GroupBox2 = New System.Windows.Forms.GroupBox()
        Me.ShutterFirmware = New System.Windows.Forms.Label()
        Me.BaseFirmware = New System.Windows.Forms.Label()
        Me.Label5 = New System.Windows.Forms.Label()
        Me.Label6 = New System.Windows.Forms.Label()
        Me.GroupBox3 = New System.Windows.Forms.GroupBox()
        Me.LowVoltage = New System.Windows.Forms.TextBox()
        Me.Label3 = New System.Windows.Forms.Label()
        Me.Label4 = New System.Windows.Forms.Label()
        Me.GroupBox5 = New System.Windows.Forms.GroupBox()
        Me.SleepTimer = New System.Windows.Forms.TextBox()
        Me.HomeBox.SuspendLayout()
        Me.ParkBox.SuspendLayout()
        Me.GroupBox4.SuspendLayout()
        Me.BatteryBox.SuspendLayout()
        Me.GroupBox1.SuspendLayout()
        Me.GroupBox2.SuspendLayout()
        Me.GroupBox3.SuspendLayout()
        Me.GroupBox5.SuspendLayout()
        Me.SuspendLayout()
        '
        'OK_Button
        '
        Me.OK_Button.Location = New System.Drawing.Point(12, 208)
        Me.OK_Button.Name = "OK_Button"
        Me.OK_Button.Size = New System.Drawing.Size(66, 27)
        Me.OK_Button.TabIndex = 0
        Me.OK_Button.Text = "OK"
        Me.OK_Button.UseVisualStyleBackColor = True
        '
        'CalibrateButton
        '
        Me.CalibrateButton.Location = New System.Drawing.Point(161, 208)
        Me.CalibrateButton.Name = "CalibrateButton"
        Me.CalibrateButton.Size = New System.Drawing.Size(60, 27)
        Me.CalibrateButton.TabIndex = 1
        Me.CalibrateButton.Text = "Calibrate"
        Me.CalibrateButton.UseVisualStyleBackColor = True
        '
        'HomeBox
        '
        Me.HomeBox.Controls.Add(Me.HomePosition)
        Me.HomeBox.Location = New System.Drawing.Point(12, 12)
        Me.HomeBox.Name = "HomeBox"
        Me.HomeBox.Size = New System.Drawing.Size(96, 50)
        Me.HomeBox.TabIndex = 2
        Me.HomeBox.TabStop = False
        Me.HomeBox.Text = "Home Azimuth"
        '
        'HomePosition
        '
        Me.HomePosition.Location = New System.Drawing.Point(10, 19)
        Me.HomePosition.Name = "HomePosition"
        Me.HomePosition.Size = New System.Drawing.Size(69, 20)
        Me.HomePosition.TabIndex = 3
        '
        'ParkBox
        '
        Me.ParkBox.Controls.Add(Me.GroupBox4)
        Me.ParkBox.Controls.Add(Me.ParkAzimuth)
        Me.ParkBox.Location = New System.Drawing.Point(12, 68)
        Me.ParkBox.Name = "ParkBox"
        Me.ParkBox.Size = New System.Drawing.Size(96, 46)
        Me.ParkBox.TabIndex = 3
        Me.ParkBox.TabStop = False
        Me.ParkBox.Text = "Park Azimuth"
        '
        'GroupBox4
        '
        Me.GroupBox4.Controls.Add(Me.TextBox1)
        Me.GroupBox4.Location = New System.Drawing.Point(2, 45)
        Me.GroupBox4.Name = "GroupBox4"
        Me.GroupBox4.Size = New System.Drawing.Size(96, 46)
        Me.GroupBox4.TabIndex = 12
        Me.GroupBox4.TabStop = False
        Me.GroupBox4.Text = "Park Azimuth"
        '
        'TextBox1
        '
        Me.TextBox1.Location = New System.Drawing.Point(6, 19)
        Me.TextBox1.Name = "TextBox1"
        Me.TextBox1.Size = New System.Drawing.Size(73, 20)
        Me.TextBox1.TabIndex = 4
        '
        'ParkAzimuth
        '
        Me.ParkAzimuth.Location = New System.Drawing.Point(6, 19)
        Me.ParkAzimuth.Name = "ParkAzimuth"
        Me.ParkAzimuth.Size = New System.Drawing.Size(73, 20)
        Me.ParkAzimuth.TabIndex = 4
        '
        'Cancel_Button
        '
        Me.Cancel_Button.Location = New System.Drawing.Point(229, 209)
        Me.Cancel_Button.Name = "Cancel_Button"
        Me.Cancel_Button.Size = New System.Drawing.Size(60, 26)
        Me.Cancel_Button.TabIndex = 4
        Me.Cancel_Button.Text = "Cancel"
        Me.Cancel_Button.UseVisualStyleBackColor = True
        '
        'BatteryBox
        '
        Me.BatteryBox.Controls.Add(Me.ShutterVolts)
        Me.BatteryBox.Controls.Add(Me.BaseVolts)
        Me.BatteryBox.Controls.Add(Me.Label2)
        Me.BatteryBox.Controls.Add(Me.Label1)
        Me.BatteryBox.Location = New System.Drawing.Point(114, 14)
        Me.BatteryBox.Name = "BatteryBox"
        Me.BatteryBox.Size = New System.Drawing.Size(107, 60)
        Me.BatteryBox.TabIndex = 5
        Me.BatteryBox.TabStop = False
        Me.BatteryBox.Text = "Battery Voltage"
        '
        'ShutterVolts
        '
        Me.ShutterVolts.AutoSize = True
        Me.ShutterVolts.Location = New System.Drawing.Point(53, 37)
        Me.ShutterVolts.Name = "ShutterVolts"
        Me.ShutterVolts.Size = New System.Drawing.Size(16, 13)
        Me.ShutterVolts.TabIndex = 3
        Me.ShutterVolts.Text = "   "
        '
        'BaseVolts
        '
        Me.BaseVolts.AutoSize = True
        Me.BaseVolts.Location = New System.Drawing.Point(52, 16)
        Me.BaseVolts.Name = "BaseVolts"
        Me.BaseVolts.Size = New System.Drawing.Size(16, 13)
        Me.BaseVolts.TabIndex = 2
        Me.BaseVolts.Text = "   "
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(6, 37)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(41, 13)
        Me.Label2.TabIndex = 1
        Me.Label2.Text = "Shutter"
        '
        'Label1
        '
        Me.Label1.AutoSize = True
        Me.Label1.Location = New System.Drawing.Point(6, 16)
        Me.Label1.Name = "Label1"
        Me.Label1.Size = New System.Drawing.Size(31, 13)
        Me.Label1.TabIndex = 0
        Me.Label1.Text = "Base"
        '
        'HomeButton
        '
        Me.HomeButton.Location = New System.Drawing.Point(86, 208)
        Me.HomeButton.Name = "HomeButton"
        Me.HomeButton.Size = New System.Drawing.Size(66, 27)
        Me.HomeButton.TabIndex = 6
        Me.HomeButton.Text = "Home"
        Me.HomeButton.UseVisualStyleBackColor = True
        '
        'GroupBox1
        '
        Me.GroupBox1.Controls.Add(Me.StatusLine)
        Me.GroupBox1.Location = New System.Drawing.Point(114, 80)
        Me.GroupBox1.Name = "GroupBox1"
        Me.GroupBox1.Size = New System.Drawing.Size(183, 48)
        Me.GroupBox1.TabIndex = 7
        Me.GroupBox1.TabStop = False
        Me.GroupBox1.Text = "Dome Status"
        '
        'StatusLine
        '
        Me.StatusLine.AutoSize = True
        Me.StatusLine.Location = New System.Drawing.Point(6, 21)
        Me.StatusLine.Name = "StatusLine"
        Me.StatusLine.Size = New System.Drawing.Size(19, 13)
        Me.StatusLine.TabIndex = 8
        Me.StatusLine.Text = "    "
        '
        'ReverseCheckbox
        '
        Me.ReverseCheckbox.AutoSize = True
        Me.ReverseCheckbox.Location = New System.Drawing.Point(19, 171)
        Me.ReverseCheckbox.Name = "ReverseCheckbox"
        Me.ReverseCheckbox.Size = New System.Drawing.Size(72, 17)
        Me.ReverseCheckbox.TabIndex = 9
        Me.ReverseCheckbox.Text = "Reversed"
        Me.ReverseCheckbox.UseVisualStyleBackColor = True
        '
        'GroupBox2
        '
        Me.GroupBox2.Controls.Add(Me.ShutterFirmware)
        Me.GroupBox2.Controls.Add(Me.BaseFirmware)
        Me.GroupBox2.Controls.Add(Me.Label5)
        Me.GroupBox2.Controls.Add(Me.Label6)
        Me.GroupBox2.Location = New System.Drawing.Point(116, 134)
        Me.GroupBox2.Name = "GroupBox2"
        Me.GroupBox2.Size = New System.Drawing.Size(182, 60)
        Me.GroupBox2.TabIndex = 10
        Me.GroupBox2.TabStop = False
        Me.GroupBox2.Text = "Arduino Firmware Versions"
        '
        'ShutterFirmware
        '
        Me.ShutterFirmware.AutoSize = True
        Me.ShutterFirmware.Location = New System.Drawing.Point(53, 37)
        Me.ShutterFirmware.Name = "ShutterFirmware"
        Me.ShutterFirmware.Size = New System.Drawing.Size(16, 13)
        Me.ShutterFirmware.TabIndex = 3
        Me.ShutterFirmware.Text = "   "
        '
        'BaseFirmware
        '
        Me.BaseFirmware.AutoSize = True
        Me.BaseFirmware.Location = New System.Drawing.Point(52, 16)
        Me.BaseFirmware.Name = "BaseFirmware"
        Me.BaseFirmware.Size = New System.Drawing.Size(16, 13)
        Me.BaseFirmware.TabIndex = 2
        Me.BaseFirmware.Text = "   "
        '
        'Label5
        '
        Me.Label5.AutoSize = True
        Me.Label5.Location = New System.Drawing.Point(6, 37)
        Me.Label5.Name = "Label5"
        Me.Label5.Size = New System.Drawing.Size(41, 13)
        Me.Label5.TabIndex = 1
        Me.Label5.Text = "Shutter"
        '
        'Label6
        '
        Me.Label6.AutoSize = True
        Me.Label6.Location = New System.Drawing.Point(6, 16)
        Me.Label6.Name = "Label6"
        Me.Label6.Size = New System.Drawing.Size(31, 13)
        Me.Label6.TabIndex = 0
        Me.Label6.Text = "Base"
        '
        'GroupBox3
        '
        Me.GroupBox3.Controls.Add(Me.LowVoltage)
        Me.GroupBox3.Controls.Add(Me.Label3)
        Me.GroupBox3.Controls.Add(Me.Label4)
        Me.GroupBox3.Location = New System.Drawing.Point(229, 14)
        Me.GroupBox3.Name = "GroupBox3"
        Me.GroupBox3.Size = New System.Drawing.Size(67, 60)
        Me.GroupBox3.TabIndex = 11
        Me.GroupBox3.TabStop = False
        Me.GroupBox3.Text = "Shutter Safety"
        '
        'LowVoltage
        '
        Me.LowVoltage.Location = New System.Drawing.Point(6, 32)
        Me.LowVoltage.Name = "LowVoltage"
        Me.LowVoltage.Size = New System.Drawing.Size(54, 20)
        Me.LowVoltage.TabIndex = 6
        '
        'Label3
        '
        Me.Label3.AutoSize = True
        Me.Label3.Location = New System.Drawing.Point(53, 37)
        Me.Label3.Name = "Label3"
        Me.Label3.Size = New System.Drawing.Size(16, 13)
        Me.Label3.TabIndex = 3
        Me.Label3.Text = "   "
        '
        'Label4
        '
        Me.Label4.AutoSize = True
        Me.Label4.Location = New System.Drawing.Point(52, 16)
        Me.Label4.Name = "Label4"
        Me.Label4.Size = New System.Drawing.Size(16, 13)
        Me.Label4.TabIndex = 2
        Me.Label4.Text = "   "
        '
        'GroupBox5
        '
        Me.GroupBox5.Controls.Add(Me.SleepTimer)
        Me.GroupBox5.Location = New System.Drawing.Point(14, 120)
        Me.GroupBox5.Name = "GroupBox5"
        Me.GroupBox5.Size = New System.Drawing.Size(96, 46)
        Me.GroupBox5.TabIndex = 12
        Me.GroupBox5.TabStop = False
        Me.GroupBox5.Text = "Sleep Timer"
        '
        'SleepTimer
        '
        Me.SleepTimer.Location = New System.Drawing.Point(6, 19)
        Me.SleepTimer.Name = "SleepTimer"
        Me.SleepTimer.Size = New System.Drawing.Size(73, 20)
        Me.SleepTimer.TabIndex = 4
        '
        'SetupForm
        '
        Me.AcceptButton = Me.OK_Button
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(306, 256)
        Me.Controls.Add(Me.GroupBox5)
        Me.Controls.Add(Me.GroupBox3)
        Me.Controls.Add(Me.GroupBox2)
        Me.Controls.Add(Me.ReverseCheckbox)
        Me.Controls.Add(Me.GroupBox1)
        Me.Controls.Add(Me.HomeButton)
        Me.Controls.Add(Me.BatteryBox)
        Me.Controls.Add(Me.Cancel_Button)
        Me.Controls.Add(Me.ParkBox)
        Me.Controls.Add(Me.HomeBox)
        Me.Controls.Add(Me.CalibrateButton)
        Me.Controls.Add(Me.OK_Button)
        Me.Name = "SetupForm"
        Me.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent
        Me.Text = "NexDome ASCOM Setup 1.12"
        Me.HomeBox.ResumeLayout(False)
        Me.HomeBox.PerformLayout()
        Me.ParkBox.ResumeLayout(False)
        Me.ParkBox.PerformLayout()
        Me.GroupBox4.ResumeLayout(False)
        Me.GroupBox4.PerformLayout()
        Me.BatteryBox.ResumeLayout(False)
        Me.BatteryBox.PerformLayout()
        Me.GroupBox1.ResumeLayout(False)
        Me.GroupBox1.PerformLayout()
        Me.GroupBox2.ResumeLayout(False)
        Me.GroupBox2.PerformLayout()
        Me.GroupBox3.ResumeLayout(False)
        Me.GroupBox3.PerformLayout()
        Me.GroupBox5.ResumeLayout(False)
        Me.GroupBox5.PerformLayout()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub

    Friend WithEvents OK_Button As Button
    Friend WithEvents CalibrateButton As Button
    Friend WithEvents HomeBox As GroupBox
    Friend WithEvents HomePosition As TextBox
    Friend WithEvents ParkBox As GroupBox
    Friend WithEvents ParkAzimuth As TextBox
    Friend WithEvents Cancel_Button As Button
    Friend WithEvents BatteryBox As GroupBox
    Friend WithEvents ShutterVolts As Label
    Friend WithEvents BaseVolts As Label
    Friend WithEvents Label2 As Label
    Friend WithEvents Label1 As Label
    Friend WithEvents HomeButton As Button
    Friend WithEvents GroupBox1 As GroupBox
    Friend WithEvents StatusLine As Label
    Friend WithEvents ReverseCheckbox As CheckBox
    Friend WithEvents GroupBox2 As GroupBox
    Friend WithEvents ShutterFirmware As Label
    Friend WithEvents BaseFirmware As Label
    Friend WithEvents Label5 As Label
    Friend WithEvents Label6 As Label
    Friend WithEvents GroupBox3 As GroupBox
    Friend WithEvents LowVoltage As TextBox
    Friend WithEvents Label3 As Label
    Friend WithEvents Label4 As Label
    Friend WithEvents GroupBox4 As GroupBox
    Friend WithEvents TextBox1 As TextBox
    Friend WithEvents GroupBox5 As GroupBox
    Friend WithEvents SleepTimer As TextBox
End Class
