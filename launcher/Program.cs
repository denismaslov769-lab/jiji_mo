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
            int mb = args.Length >= 4 && int.TryParse(args[3], out var v) ? v : 1900;
            Application.Run(new PackProgressForm(args[1], args[2], mb));
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
