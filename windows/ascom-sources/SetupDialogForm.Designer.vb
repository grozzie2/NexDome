<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class SetupForm
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
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
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Me.OK_Button = New System.Windows.Forms.Button()
        Me.CalibrateButton = New System.Windows.Forms.Button()
        Me.HomeBox = New System.Windows.Forms.GroupBox()
        Me.HomePosition = New System.Windows.Forms.TextBox()
        Me.ParkBox = New System.Windows.Forms.GroupBox()
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
        Me.AzimuthBox = New System.Windows.Forms.GroupBox()
        Me.DomeAzimuth = New System.Windows.Forms.Label()
        Me.SyncCheckbox = New System.Windows.Forms.CheckBox()
        Me.HomeBox.SuspendLayout()
        Me.ParkBox.SuspendLayout()
        Me.BatteryBox.SuspendLayout()
        Me.GroupBox1.SuspendLayout()
        Me.AzimuthBox.SuspendLayout()
        Me.SuspendLayout()
        '
        'OK_Button
        '
        Me.OK_Button.Location = New System.Drawing.Point(143, 172)
        Me.OK_Button.Name = "OK_Button"
        Me.OK_Button.Size = New System.Drawing.Size(68, 23)
        Me.OK_Button.TabIndex = 0
        Me.OK_Button.Text = "OK"
        Me.OK_Button.UseVisualStyleBackColor = True
        '
        'CalibrateButton
        '
        Me.CalibrateButton.Location = New System.Drawing.Point(217, 134)
        Me.CalibrateButton.Name = "CalibrateButton"
        Me.CalibrateButton.Size = New System.Drawing.Size(62, 23)
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
        Me.ParkBox.Controls.Add(Me.ParkAzimuth)
        Me.ParkBox.Location = New System.Drawing.Point(12, 68)
        Me.ParkBox.Name = "ParkBox"
        Me.ParkBox.Size = New System.Drawing.Size(96, 46)
        Me.ParkBox.TabIndex = 3
        Me.ParkBox.TabStop = False
        Me.ParkBox.Text = "Park Azimuth"
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
        Me.Cancel_Button.Location = New System.Drawing.Point(217, 172)
        Me.Cancel_Button.Name = "Cancel_Button"
        Me.Cancel_Button.Size = New System.Drawing.Size(62, 23)
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
        Me.BatteryBox.Size = New System.Drawing.Size(165, 60)
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
        Me.HomeButton.Location = New System.Drawing.Point(143, 134)
        Me.HomeButton.Name = "HomeButton"
        Me.HomeButton.Size = New System.Drawing.Size(68, 23)
        Me.HomeButton.TabIndex = 6
        Me.HomeButton.Text = "Home"
        Me.HomeButton.UseVisualStyleBackColor = True
        '
        'GroupBox1
        '
        Me.GroupBox1.Controls.Add(Me.StatusLine)
        Me.GroupBox1.Location = New System.Drawing.Point(114, 80)
        Me.GroupBox1.Name = "GroupBox1"
        Me.GroupBox1.Size = New System.Drawing.Size(165, 48)
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
        'AzimuthBox
        '
        Me.AzimuthBox.Controls.Add(Me.DomeAzimuth)
        Me.AzimuthBox.Location = New System.Drawing.Point(14, 120)
        Me.AzimuthBox.Name = "AzimuthBox"
        Me.AzimuthBox.Size = New System.Drawing.Size(94, 37)
        Me.AzimuthBox.TabIndex = 8
        Me.AzimuthBox.TabStop = False
        Me.AzimuthBox.Text = "Dome Azimuth"
        '
        'DomeAzimuth
        '
        Me.DomeAzimuth.AutoSize = True
        Me.DomeAzimuth.Location = New System.Drawing.Point(15, 16)
        Me.DomeAzimuth.Name = "DomeAzimuth"
        Me.DomeAzimuth.Size = New System.Drawing.Size(19, 13)
        Me.DomeAzimuth.TabIndex = 9
        Me.DomeAzimuth.Text = "    "
        '
        'SyncCheckbox
        '
        Me.SyncCheckbox.AutoSize = True
        Me.SyncCheckbox.Location = New System.Drawing.Point(14, 163)
        Me.SyncCheckbox.Name = "SyncCheckbox"
        Me.SyncCheckbox.Size = New System.Drawing.Size(96, 17)
        Me.SyncCheckbox.TabIndex = 9
        Me.SyncCheckbox.Text = "Sync on Home"
        Me.SyncCheckbox.UseVisualStyleBackColor = True
        '
        'SetupForm
        '
        Me.AcceptButton = Me.OK_Button
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(293, 212)
        Me.Controls.Add(Me.SyncCheckbox)
        Me.Controls.Add(Me.AzimuthBox)
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
        Me.Text = "NexDome Setup"
        Me.HomeBox.ResumeLayout(False)
        Me.HomeBox.PerformLayout()
        Me.ParkBox.ResumeLayout(False)
        Me.ParkBox.PerformLayout()
        Me.BatteryBox.ResumeLayout(False)
        Me.BatteryBox.PerformLayout()
        Me.GroupBox1.ResumeLayout(False)
        Me.GroupBox1.PerformLayout()
        Me.AzimuthBox.ResumeLayout(False)
        Me.AzimuthBox.PerformLayout()
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
    Friend WithEvents AzimuthBox As GroupBox
    Friend WithEvents DomeAzimuth As Label
    Friend WithEvents SyncCheckbox As CheckBox
End Class
