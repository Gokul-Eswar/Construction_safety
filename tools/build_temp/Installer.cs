using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;

namespace SentinelInstaller
{
    class Program
    {
        static void Main(string[] args)
        {
            string tempPath = Path.Combine(Path.GetTempPath(), "SentinelInstaller_" + Guid.NewGuid().ToString().Substring(0, 8));
            string zipPath = Path.Combine(tempPath, "payload.zip");

            try 
            {
                if (Directory.Exists(tempPath)) Directory.Delete(tempPath, true);
                Directory.CreateDirectory(tempPath);

                // 1. Extract embedded zip
                using (Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("payload.zip"))
                using (FileStream fileStream = new FileStream(zipPath, FileMode.Create))
                {
                    if (stream == null) throw new Exception("Embedded resource not found.");
                    stream.CopyTo(fileStream);
                }

                // 2. Unzip using PowerShell (Native .NET 4.0 zip is tricky, PS is reliable on modern Windows)
                ProcessStartInfo psiUnzip = new ProcessStartInfo();
                psiUnzip.FileName = "powershell";
                psiUnzip.Arguments = "-NoProfile -Command \"Expand-Archive -Path '" + zipPath + "' -DestinationPath '" + tempPath + "' -Force\" ";
                psiUnzip.WindowStyle = ProcessWindowStyle.Hidden;
                
                Process unzipProc = Process.Start(psiUnzip);
                unzipProc.WaitForExit();

                if (unzipProc.ExitCode != 0) throw new Exception("Extraction failed.");

                // 3. Run Setup.bat
                ProcessStartInfo psiSetup = new ProcessStartInfo();
                psiSetup.FileName = Path.Combine(tempPath, "Setup.bat");
                psiSetup.WorkingDirectory = tempPath;
                psiSetup.UseShellExecute = true; // Use shell to handle .bat
                
                Process setupProc = Process.Start(psiSetup);
                // We don't wait for setup to finish, it forks its own GUI. 
                // But if we exit, tempPath might be locked? 
                // Actually, Setup.bat runs PowerShell script which stays open.
                // If we exit this wrapper, the temp files remain?
                // We can't easily clean up temp files if we spawn a detached process.
                // For an installer, leaving temp files is acceptable (Windows cleans %TEMP% eventually).
            }
            catch (Exception ex)
            {
                // Simple error dialog via generic Windows command if GUI fails
                // Or just silent fail.
            }
        }
    }
}
