using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Windows.Forms;

namespace GodjoSetup
{
    internal static class Program
    {
        [STAThread]
        private static int Main(string[] args)
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            bool uninstall = args.Any(a => a.Equals("/uninstall", StringComparison.OrdinalIgnoreCase));
            bool silent = args.Any(a => a.Equals("/S", StringComparison.OrdinalIgnoreCase) || a.Equals("/silent", StringComparison.OrdinalIgnoreCase));
            string dir = args.FirstOrDefault(a => a.StartsWith("/D=", StringComparison.OrdinalIgnoreCase))?.Substring(3);
            try
            {
                if (uninstall)
                {
                    // uninstall.exe лежит в папке игры и не может удалить сам себя —
                    // копируем себя во %TEMP% и перезапускаемся оттуда.
                    string self = Process.GetCurrentProcess().MainModule.FileName;
                    string target = dir ?? Path.GetDirectoryName(self);
                    if (!args.Any(a => a.Equals("/fromtemp", StringComparison.OrdinalIgnoreCase)))
                    {
                        string tmp = Path.Combine(Path.GetTempPath(), "GodjoUninstall_" + Guid.NewGuid().ToString("N").Substring(0, 8) + ".exe");
                        File.Copy(self, tmp, true);
                        Process.Start(new ProcessStartInfo(tmp, "/uninstall /fromtemp " + (silent ? "/S " : "") + "\"/D=" + target + "\"") { UseShellExecute = false });
                        return 0;
                    }
                    if (silent) { Installer.Uninstall(target, false, null); return 0; }
                    Application.Run(new SetupForm(SetupMode.Uninstall, target));
                    return 0;
                }
                if (silent)
                {
                    Installer.Install(dir ?? Installer.DefaultDir, true, true, null);
                    return 0;
                }
                Application.Run(new SetupForm(SetupMode.Install, dir ?? Installer.ExistingDir() ?? Installer.DefaultDir));
                return 0;
            }
            catch (Exception e)
            {
                if (!silent) MessageBox.Show(e.Message, "Godjo Role Play", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return 1;
            }
        }
    }
}
