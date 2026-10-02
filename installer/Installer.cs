using System;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Reflection;
using Microsoft.Win32;

namespace GodjoSetup
{
    /// <summary>Логика установки/удаления. Ставит в профиль пользователя (HKCU) — без UAC.</summary>
    internal static class Installer
    {
        public const string AppName = "Godjo Role Play";
        public const string ExeName = "GodjoLauncher.exe";
        const string UninstKey = @"Software\Microsoft\Windows\CurrentVersion\Uninstall\GodjoRP";
        const string WebView2Id = "{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}";
        const string WebView2Url = "https://go.microsoft.com/fwlink/p/?LinkId=2124703";

        public static string Version => Assembly.GetExecutingAssembly().GetName().Version.ToString(3);
        public static string DefaultDir => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Programs", "GodjoRP");
        static string DesktopLnk => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), AppName + ".lnk");
        static string MenuDir => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Programs), AppName);

        public static string ExistingDir()
        {
            using (var k = Registry.CurrentUser.OpenSubKey(UninstKey))
            {
                var d = k?.GetValue("InstallLocation") as string;
                return !string.IsNullOrEmpty(d) && Directory.Exists(d) ? d : null;
            }
        }

        public static long PayloadSize()
        {
            using (var s = Assembly.GetExecutingAssembly().GetManifestResourceStream("payload.zip"))
            using (var z = new ZipArchive(s, ZipArchiveMode.Read))
                return z.Entries.Sum(e => e.Length);
        }

        public static long FreeSpace(string dir)
        {
            try { return new DriveInfo(Path.GetPathRoot(Path.GetFullPath(dir))).AvailableFreeSpace; } catch { return long.MaxValue; }
        }

        public static bool HasWebView2()
        {
            foreach (var root in new[] { Registry.LocalMachine, Registry.CurrentUser })
                foreach (var path in new[] { @"SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\", @"SOFTWARE\Microsoft\EdgeUpdate\Clients\" })
                    using (var k = root.OpenSubKey(path + WebView2Id))
                    {
                        var v = k?.GetValue("pv") as string;
                        if (!string.IsNullOrEmpty(v) && v != "0.0.0.0") return true;
                    }
            return false;
        }

        static void KillLauncher(string dir)
        {
            foreach (var p in Process.GetProcessesByName(Path.GetFileNameWithoutExtension(ExeName)))
            {
                try
                {
                    if (p.MainModule.FileName.StartsWith(dir, StringComparison.OrdinalIgnoreCase)) { p.Kill(); p.WaitForExit(5000); }
                }
                catch { }
            }
        }

        /// <param name="report">(текст, процент 0..100)</param>
        public static void Install(string dir, bool desktop, bool startMenu, Action<string, int> report)
        {
            report = report ?? ((a, b) => { });
            dir = Path.GetFullPath(dir);
            report("Подготовка…", 2);
            Directory.CreateDirectory(dir);
            KillLauncher(dir);

            // 1. файлы лаунчера
            using (var s = Assembly.GetExecutingAssembly().GetManifestResourceStream("payload.zip"))
            using (var z = new ZipArchive(s, ZipArchiveMode.Read))
            {
                long total = Math.Max(1, z.Entries.Sum(e => e.Length)), done = 0;
                var buf = new byte[1 << 16];
                foreach (var e in z.Entries)
                {
                    string dest = Path.GetFullPath(Path.Combine(dir, e.FullName));
                    if (!dest.StartsWith(dir, StringComparison.OrdinalIgnoreCase)) continue;
                    if (string.IsNullOrEmpty(e.Name)) { Directory.CreateDirectory(dest); continue; }
                    // пользовательский launcher.json не перетираем
                    if (e.Name.Equals("launcher.json", StringComparison.OrdinalIgnoreCase) && File.Exists(dest)) { done += e.Length; continue; }
                    Directory.CreateDirectory(Path.GetDirectoryName(dest));
                    using (var src = e.Open())
                    using (var dst = File.Create(dest))
                    {
                        int n;
                        while ((n = src.Read(buf, 0, buf.Length)) > 0)
                        {
                            dst.Write(buf, 0, n); done += n;
                            report("Копирование " + e.Name, 5 + (int)(done * 70 / total));
                        }
                    }
                }
            }

            // 2. деинсталлятор
            report("Регистрация в системе…", 78);
            string self = Process.GetCurrentProcess().MainModule.FileName;
            string uninst = Path.Combine(dir, "uninstall.exe");
            try { File.Copy(self, uninst, true); } catch { }
            string exe = Path.Combine(dir, ExeName);
            using (var k = Registry.CurrentUser.CreateSubKey(UninstKey))
            {
                k.SetValue("DisplayName", AppName + " Launcher");
                k.SetValue("DisplayVersion", Version);
                k.SetValue("Publisher", AppName);
                k.SetValue("DisplayIcon", exe);
                k.SetValue("InstallLocation", dir);
                k.SetValue("UninstallString", "\"" + uninst + "\" /uninstall");
                k.SetValue("QuietUninstallString", "\"" + uninst + "\" /uninstall /S");
                k.SetValue("NoModify", 1, RegistryValueKind.DWord);
                k.SetValue("NoRepair", 1, RegistryValueKind.DWord);
                k.SetValue("EstimatedSize", (int)(DirSize(dir) / 1024), RegistryValueKind.DWord);
                k.SetValue("InstallDate", DateTime.Now.ToString("yyyyMMdd"));
            }

            // 3. ярлыки
            report("Создание ярлыков…", 84);
            if (desktop) Shortcut(DesktopLnk, exe, dir, "Лаунчер Godjo Role Play");
            if (startMenu)
            {
                Directory.CreateDirectory(MenuDir);
                Shortcut(Path.Combine(MenuDir, AppName + ".lnk"), exe, dir, "Лаунчер Godjo Role Play");
                Shortcut(Path.Combine(MenuDir, "Удалить " + AppName + ".lnk"), uninst, dir, "Удалить лаунчер", "/uninstall");
            }

            // 4. WebView2 Runtime (нужен лаунчеру для интерфейса)
            if (!HasWebView2())
            {
                report("Загрузка Microsoft WebView2 Runtime…", 88);
                try
                {
                    ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;
                    string tmp = Path.Combine(Path.GetTempPath(), "MicrosoftEdgeWebview2Setup.exe");
                    using (var wc = new WebClient()) wc.DownloadFile(WebView2Url, tmp);
                    report("Установка Microsoft WebView2 Runtime…", 93);
                    var p = Process.Start(new ProcessStartInfo(tmp, "/silent /install") { UseShellExecute = true });
                    p?.WaitForExit(10 * 60 * 1000);
                }
                catch { /* лаунчер сам предложит скачать WebView2 при запуске */ }
            }
            report("Готово!", 100);
        }

        public static void Uninstall(string dir, bool removeUserData, Action<string, int> report)
        {
            report = report ?? ((a, b) => { });
            report("Закрытие лаунчера…", 5);
            KillLauncher(dir);
            report("Удаление файлов…", 30);
            foreach (var f in new[] { ExeName, "launcher.json", "uninstall.exe", "WebView2Loader.dll" })
                try { File.Delete(Path.Combine(dir, f)); } catch { }
            foreach (var d in new[] { "client", "GodjoLauncher.exe.WebView2" })
                try { if (Directory.Exists(Path.Combine(dir, d))) Directory.Delete(Path.Combine(dir, d), true); } catch { }
            try { if (!Directory.EnumerateFileSystemEntries(dir).Any()) Directory.Delete(dir); } catch { }
            report("Удаление ярлыков…", 60);
            try { File.Delete(DesktopLnk); } catch { }
            try { if (Directory.Exists(MenuDir)) Directory.Delete(MenuDir, true); } catch { }
            report("Очистка реестра…", 80);
            try { Registry.CurrentUser.DeleteSubKeyTree(UninstKey, false); } catch { }
            if (removeUserData)
                try { Directory.Delete(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "GodjoLauncher"), true); } catch { }
            // временную копию деинсталлятора удаляем после выхода
            try
            {
                string self = Process.GetCurrentProcess().MainModule.FileName;
                if (self.StartsWith(Path.GetTempPath(), StringComparison.OrdinalIgnoreCase))
                    Process.Start(new ProcessStartInfo("cmd.exe", "/c ping 127.0.0.1 -n 3 > nul & del /f /q \"" + self + "\"") { CreateNoWindow = true, UseShellExecute = false });
            }
            catch { }
            report("Готово", 100);
        }

        static long DirSize(string dir)
        {
            try { return new DirectoryInfo(dir).EnumerateFiles("*", SearchOption.AllDirectories).Sum(f => f.Length); } catch { return 0; }
        }

        static void Shortcut(string lnk, string target, string workDir, string desc, string args = "")
        {
            try
            {
                var t = Type.GetTypeFromProgID("WScript.Shell");
                object shell = Activator.CreateInstance(t);
                object sc = t.InvokeMember("CreateShortcut", BindingFlags.InvokeMethod, null, shell, new object[] { lnk });
                var st = sc.GetType();
                st.InvokeMember("TargetPath", BindingFlags.SetProperty, null, sc, new object[] { target });
                st.InvokeMember("Arguments", BindingFlags.SetProperty, null, sc, new object[] { args });
                st.InvokeMember("WorkingDirectory", BindingFlags.SetProperty, null, sc, new object[] { workDir });
                st.InvokeMember("Description", BindingFlags.SetProperty, null, sc, new object[] { desc });
                st.InvokeMember("IconLocation", BindingFlags.SetProperty, null, sc, new object[] { target + ",0" });
                st.InvokeMember("Save", BindingFlags.InvokeMethod, null, sc, null);
            }
            catch { }
        }
    }
}
