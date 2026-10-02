namespace GodjoLauncher;

internal static class Program
{
    [STAThread]
    private static void Main(string[] args)
    {
        // режим для администратора: упаковать свою чистую GTA SA + SA-MP в сборку для хостинга
        if (args.Length >= 3 && args[0] == "--make-game-pack")
        {
            ApplicationConfiguration.Initialize();
            try
            {
                int mb = args.Length >= 4 && int.TryParse(args[3], out var v) ? v : 1900;
                var res = GameDownloader.MakePack(args[1], args[2], mb, s => Log.Write(s));
                MessageBox.Show(res, "Godjo — сборка игры", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message, "Godjo — сборка игры", MessageBoxButtons.OK, MessageBoxIcon.Error); }
            return;
        }
        using var mutex = new Mutex(true, "GodjoLauncher_SingleInstance", out bool created);
        if (!created)
        {
            MessageBox.Show("Лаунчер уже запущен.", "Godjo Role Play", MessageBoxButtons.OK, MessageBoxIcon.Information);
            return;
        }
        System.Text.Encoding.RegisterProvider(System.Text.CodePagesEncodingProvider.Instance);
        ApplicationConfiguration.Initialize();
        Application.Run(new MainForm());
    }
}
