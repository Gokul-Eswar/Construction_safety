Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

# --- Configuration ---
$AppName = "Sentinel Safety System"
$DefaultInstallDir = "C:\SentinelSafety"
$SourceDir = Get-Item -LiteralPath (Resolve-Path "$PSScriptRoot\..").Path

# --- GUI Setup ---
$Form = New-Object System.Windows.Forms.Form
$Form.Text = "$AppName Setup"
$Form.Size = New-Object System.Drawing.Size(600, 450)
$Form.StartPosition = "CenterScreen"
$Form.FormBorderStyle = "FixedDialog"
$Form.MaximizeBox = $false
$Form.MinimizeBox = $true
$Form.Icon = [System.Drawing.Icon]::ExtractAssociatedIcon($PSHOME + "\powershell.exe")

# --- Styles ---
$FontTitle = New-Object System.Drawing.Font("Segoe UI", 16, [System.Drawing.FontStyle]::Bold)
$FontHeader = New-Object System.Drawing.Font("Segoe UI", 12, [System.Drawing.FontStyle]::Regular)
$FontNormal = New-Object System.Drawing.Font("Segoe UI", 9, [System.Drawing.FontStyle]::Regular)

# --- Panels (Pages) ---
$PanelWelcome = New-Object System.Windows.Forms.Panel
$PanelWelcome.Dock = "Fill"
$PanelDir = New-Object System.Windows.Forms.Panel
$PanelDir.Dock = "Fill"
$PanelInstall = New-Object System.Windows.Forms.Panel
$PanelInstall.Dock = "Fill"
$PanelFinish = New-Object System.Windows.Forms.Panel
$PanelFinish.Dock = "Fill"

# --- Page 1: Welcome ---
$LblWelcomeTitle = New-Object System.Windows.Forms.Label
$LblWelcomeTitle.Text = "Welcome to the $AppName Setup Wizard"
$LblWelcomeTitle.Font = $FontTitle
$LblWelcomeTitle.AutoSize = $true
$LblWelcomeTitle.Location = New-Object System.Drawing.Point(20, 20)

$LblWelcomeDesc = New-Object System.Windows.Forms.Label
$LblWelcomeDesc.Text = "This wizard will guide you through the installation of $AppName.`n`nIt will set up the necessary files and shortcuts on your computer.`n`nClick Next to continue."
$LblWelcomeDesc.Font = $FontHeader
$LblWelcomeDesc.Size = New-Object System.Drawing.Size(540, 200)
$LblWelcomeDesc.Location = New-Object System.Drawing.Point(25, 80)

$PanelWelcome.Controls.Add($LblWelcomeTitle)
$PanelWelcome.Controls.Add($LblWelcomeDesc)

# --- Page 2: Directory ---
$LblDirTitle = New-Object System.Windows.Forms.Label
$LblDirTitle.Text = "Select Installation Folder"
$LblDirTitle.Font = $FontTitle
$LblDirTitle.AutoSize = $true
$LblDirTitle.Location = New-Object System.Drawing.Point(20, 20)

$TxtDir = New-Object System.Windows.Forms.TextBox
$TxtDir.Text = $DefaultInstallDir
$TxtDir.Font = $FontNormal
$TxtDir.Size = New-Object System.Drawing.Size(400, 30)
$TxtDir.Location = New-Object System.Drawing.Point(25, 100)

$BtnBrowse = New-Object System.Windows.Forms.Button
$BtnBrowse.Text = "Browse..."
$BtnBrowse.Location = New-Object System.Drawing.Point(435, 99)
$BtnBrowse.Add_Click({
    $FolderBrowser = New-Object System.Windows.Forms.FolderBrowserDialog
    if ($FolderBrowser.ShowDialog() -eq "OK") {
        $TxtDir.Text = $FolderBrowser.SelectedPath
    }
})

$LblDirDesc = New-Object System.Windows.Forms.Label
$LblDirDesc.Text = "Setup will install $AppName in the following folder."
$LblDirDesc.Font = $FontNormal
$LblDirDesc.AutoSize = $true
$LblDirDesc.Location = New-Object System.Drawing.Point(25, 75)

$PanelDir.Controls.Add($LblDirTitle)
$PanelDir.Controls.Add($TxtDir)
$PanelDir.Controls.Add($BtnBrowse)
$PanelDir.Controls.Add($LblDirDesc)

# --- Page 3: Installing ---
$LblInstallTitle = New-Object System.Windows.Forms.Label
$LblInstallTitle.Text = "Installing..."
$LblInstallTitle.Font = $FontTitle
$LblInstallTitle.AutoSize = $true
$LblInstallTitle.Location = New-Object System.Drawing.Point(20, 20)

$ProgressBar = New-Object System.Windows.Forms.ProgressBar
$ProgressBar.Size = New-Object System.Drawing.Size(540, 30)
$ProgressBar.Location = New-Object System.Drawing.Point(25, 100)

$LblStatus = New-Object System.Windows.Forms.Label
$LblStatus.Text = "Preparing..."
$LblStatus.Font = $FontNormal
$LblStatus.AutoSize = $true
$LblStatus.Location = New-Object System.Drawing.Point(25, 80)

$TxtLog = New-Object System.Windows.Forms.TextBox
$TxtLog.Multiline = $true
$TxtLog.ScrollBars = "Vertical"
$TxtLog.ReadOnly = $true
$TxtLog.Size = New-Object System.Drawing.Size(540, 200)
$TxtLog.Location = New-Object System.Drawing.Point(25, 140)
$TxtLog.Font = New-Object System.Drawing.Font("Consolas", 8)

$PanelInstall.Controls.Add($LblInstallTitle)
$PanelInstall.Controls.Add($ProgressBar)
$PanelInstall.Controls.Add($LblStatus)
$PanelInstall.Controls.Add($TxtLog)

# --- Page 4: Finish ---
$LblFinishTitle = New-Object System.Windows.Forms.Label
$LblFinishTitle.Text = "Installation Complete"
$LblFinishTitle.Font = $FontTitle
$LblFinishTitle.AutoSize = $true
$LblFinishTitle.Location = New-Object System.Drawing.Point(20, 20)

$LblFinishDesc = New-Object System.Windows.Forms.Label
$LblFinishDesc.Text = "$AppName has been installed on your computer.`n`nClick Finish to exit Setup."
$LblFinishDesc.Font = $FontHeader
$LblFinishDesc.Size = New-Object System.Drawing.Size(540, 100)
$LblFinishDesc.Location = New-Object System.Drawing.Point(25, 80)

$ChkLaunch = New-Object System.Windows.Forms.CheckBox
$ChkLaunch.Text = "Launch $AppName now"
$ChkLaunch.Font = $FontHeader
$ChkLaunch.Checked = $true
$ChkLaunch.AutoSize = $true
$ChkLaunch.Location = New-Object System.Drawing.Point(25, 200)

$PanelFinish.Controls.Add($LblFinishTitle)
$PanelFinish.Controls.Add($LblFinishDesc)
$PanelFinish.Controls.Add($ChkLaunch)


# --- Navigation Buttons ---
$BtnNext = New-Object System.Windows.Forms.Button
$BtnNext.Text = "Next >"
$BtnNext.Location = New-Object System.Drawing.Point(400, 370)
$BtnNext.Size = New-Object System.Drawing.Size(80, 30)

$BtnCancel = New-Object System.Windows.Forms.Button
$BtnCancel.Text = "Cancel"
$BtnCancel.Location = New-Object System.Drawing.Point(490, 370)
$BtnCancel.Size = New-Object System.Drawing.Size(80, 30)
$BtnCancel.Add_Click({ $Form.Close() })

$BtnBack = New-Object System.Windows.Forms.Button
$BtnBack.Text = "< Back"
$BtnBack.Location = New-Object System.Drawing.Point(310, 370)
$BtnBack.Size = New-Object System.Drawing.Size(80, 30)
$BtnBack.Enabled = $false

$Form.Controls.Add($BtnNext)
$Form.Controls.Add($BtnCancel)
$Form.Controls.Add($BtnBack)

# --- Logic ---
$Form.Controls.Add($PanelWelcome)
$Form.Controls.Add($PanelDir)
$Form.Controls.Add($PanelInstall)
$Form.Controls.Add($PanelFinish)

# Hide all except Welcome
$PanelDir.Visible = $false
$PanelInstall.Visible = $false
$PanelFinish.Visible = $false

$CurrentStep = 1

$BtnBack.Add_Click({
    if ($script:CurrentStep -eq 2) {
        $PanelDir.Visible = $false
        $PanelWelcome.Visible = $true
        $BtnBack.Enabled = $false
        $script:CurrentStep = 1
    }
})

$DoInstall = {
    $PanelDir.Visible = $false
    $PanelInstall.Visible = $true
    $BtnBack.Enabled = $false
    $BtnNext.Enabled = $false
    $BtnCancel.Enabled = $false
    $script:CurrentStep = 3

    # START INSTALLATION
    $Dest = $TxtDir.Text
    $Log = { param($msg) $TxtLog.AppendText("$msg`r`n"); $Form.Refresh() }
    
    # Exclude list
    $Excludes = @("installer", ".git", "build", "node_modules", ".gemini", "tmp", "coverage")

    try {
        if (-not (Test-Path $Dest)) {
            New-Item -ItemType Directory -Force -Path $Dest | Out-Null
            &$Log "Created directory: $Dest"
        }

        # Check for existing installation
        if (Test-Path "$Dest\start_system.bat") {
             &$Log "Existing installation detected. Updating files..."
        }

        # --- Phase 1: File Discovery (0-5%) ---
        $LblStatus.Text = "Analyzing files..."
        &$Log "Scanning source files..."
        
        # Robust filtering: Get all items, then filter based on relative path
        # This prevents "installer" contents from being included if "installer" folder is excluded
        $AllItemsRaw = Get-ChildItem -Path $SourceDir -Recurse
        $AllItems = $AllItemsRaw | Where-Object {
            $ItemPath = $_.FullName
            # Safe relative path calculation
            if ($ItemPath.StartsWith($SourceDir.FullName)) {
                $RelPath = $ItemPath.Substring($SourceDir.FullName.Length).TrimStart('\', '/')
            } else {
                return $true # Should not happen, but keep if weird
            }
            
            # Check against excludes
            # We treat excludes as top-level folders/files relative to source, or any file name matching
            $FirstPart = $RelPath.Split('\/')[0]
            if ($Excludes -contains $FirstPart) { return $false }
            if ($Excludes -contains $_.Name) { return $false } # Also exclude matching names deeper in tree (like .git)
            
            return $true
        }

        $TotalItems = $AllItems.Count
        $ProgressBar.Value = 5
        [System.Windows.Forms.Application]::DoEvents()

        # --- Phase 2: Copying (5-90%) ---
        $Count = 0
        foreach ($Item in $AllItems) {
            $Count++
            # Map $Count/$TotalItems to the 5-90 range
            if ($TotalItems -gt 0) {
                $Progress = 5 + [int](($Count / $TotalItems) * 85)
            } else {
                $Progress = 90
            }
            $ProgressBar.Value = $Progress
            
            # Robust relative path calculation
            $SourcePathStr = $SourceDir.FullName
            $ItemPathStr = $Item.FullName
            
            if ($ItemPathStr.StartsWith($SourcePathStr)) {
                $RelativePath = $ItemPathStr.Substring($SourcePathStr.Length).TrimStart('\', '/')
            } else {
                $RelativePath = $Item.Name
            }
            
            if ([string]::IsNullOrWhiteSpace($RelativePath)) { continue }

            try {
                $TargetPath = Join-Path $Dest $RelativePath
                
                if ($Item.PSIsContainer) {
                    if (-not (Test-Path $TargetPath)) {
                        New-Item -ItemType Directory -Path $TargetPath -Force | Out-Null
                    }
                } else {
                    $LblStatus.Text = "Copying: $($Item.Name)"
                    
                    # Safety: Ensure parent directory exists
                    $ParentDir = Split-Path -Parent $TargetPath
                    if (-not (Test-Path $ParentDir)) {
                        New-Item -ItemType Directory -Path $ParentDir -Force | Out-Null
                    }
                    
                    Copy-Item -Path $Item.FullName -Destination $TargetPath -Force
                }
            } catch {
                 throw "Copy failed for Item: '$ItemPathStr' to '$TargetPath'. Error: $($_.Exception.Message)"
            }
            
            if ($Count % 10 -eq 0) { [System.Windows.Forms.Application]::DoEvents() }
        }

        # --- Phase 3: System Configuration (90-95%) ---
        $ProgressBar.Value = 90
        $LblStatus.Text = "Configuring system shortcuts..."
        &$Log "Creating Desktop Shortcut..."
        $WshShell = New-Object -ComObject WScript.Shell
        $ShortcutPath = "$Home\Desktop\Sentinel Safety.lnk"
        $Shortcut = $WshShell.CreateShortcut($ShortcutPath)
        $Shortcut.TargetPath = "$Dest\start_system.bat"
        $Shortcut.WorkingDirectory = "$Dest"
        $Shortcut.IconLocation = "shell32.dll, 15"
        $Shortcut.Save()
        
        $ProgressBar.Value = 93
        &$Log "Creating Start Menu Shortcut..."
        $StartMenuPath = "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\Sentinel Safety.lnk"
        $Shortcut = $WshShell.CreateShortcut($StartMenuPath)
        $Shortcut.TargetPath = "$Dest\start_system.bat"
        $Shortcut.WorkingDirectory = "$Dest"
        $Shortcut.IconLocation = "shell32.dll, 15"
        $Shortcut.Save()

        # --- Phase 4: Finalizing (95-100%) ---
        $ProgressBar.Value = 100
        $LblStatus.Text = "Completed"
        &$Log "Installation Successful."
        
        Start-Sleep -Seconds 1
        
        $PanelInstall.Visible = $false
        $PanelFinish.Visible = $true
        $BtnNext.Text = "Finish"
        $BtnNext.Enabled = $true
        $BtnCancel.Enabled = $false
        $script:CurrentStep = 4

    } catch {
        $ErrorMsg = "Installation Failed.`nError: $($_.Exception.Message)"
        if ($Item) { $ErrorMsg += "`nLast File: $($Item.FullName)" }
        &$Log "ERROR: $ErrorMsg"
        [System.Windows.Forms.MessageBox]::Show($ErrorMsg, "Error", "OK", "Error")
        $Form.Close()
    }
}

$BtnNext.Add_Click({
    if ($script:CurrentStep -eq 1) {
        $PanelWelcome.Visible = $false
        $PanelDir.Visible = $true
        $BtnBack.Enabled = $true
        $script:CurrentStep = 2
    }
    elseif ($script:CurrentStep -eq 2) {
        # --- Pre-Install Checks ---
        $MissingReqs = @()
        
        # Check 1: Docker
        try {
            $dockerCheck = Get-Command docker -ErrorAction Stop
        } catch {
            $MissingReqs += "Docker Desktop is not installed or not in PATH."
        }

        # Check 2: NVIDIA Driver (Simple check for nvidia-smi)
        try {
            $gpuCheck = Get-Command nvidia-smi -ErrorAction Stop
        } catch {
            $MissingReqs += "NVIDIA Drivers (nvidia-smi) not found."
        }

        if ($MissingReqs.Count -gt 0) {
            $Msg = "Warning: Missing Prerequisites!`n`n" + ($MissingReqs -join "`n") + "`n`nThe system requires these to run. You can install them after this setup, but the application will not start without them.`n`nDo you want to proceed anyway?"
            $Result = [System.Windows.Forms.MessageBox]::Show($Msg, "Prerequisites Missing", "YesNo", "Warning")
            
            if ($Result -eq "No") {
                return # Stay on this page
            }
        }
        
        &$DoInstall
    }
    elseif ($script:CurrentStep -eq 4) {
        if ($ChkLaunch.Checked) {
            Start-Process "$($TxtDir.Text)\start_system.bat"
        }
        $Form.Close()
    }
})

$Form.ShowDialog()
