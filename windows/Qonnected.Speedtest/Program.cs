using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace Qonnected.Speedtest;

internal static class Program
{
    [STAThread]
    private static void Main(string[] args)
    {
        // CI-only packaging check; does not start a measurement or contact a server.
        if (args.Length == 2 && args[0] == "--verify-embedded-engine")
        {
            try
            {
                var executable = EmbeddedEngine.Resolve();
                using var stream = File.OpenRead(executable);
                if (stream.Length < 1024 || stream.ReadByte() != 'M' || stream.ReadByte() != 'Z')
                    throw new InvalidDataException("Embedded engine is not a valid PE executable.");
                File.WriteAllText(args[1], "Embedded native Windows engine extracted and PE header verified.");
            }
            catch (Exception ex)
            {
                File.WriteAllText(args[1], "FAILED: " + ex);
                Environment.ExitCode = 1;
            }
            return;
        }
        var app = new Application();
        app.Run(new SpeedtestWindow());
    }
}

internal sealed class SpeedtestWindow : Window
{
    #if ONVI_EDITION
    private readonly bool customer = true;
#else
    private readonly bool customer = false;
#endif
    private readonly TextBox server = new() { Text = "speedtest.nikhef.qonnected.net", MinWidth = 260 };
    private readonly ComboBox protocol = new() { MinWidth = 150 };
    private readonly TextBox port = new() { Text = "24601", Width = 85 };
    private readonly ComboBox direction = new() { MinWidth = 160 };
    private readonly TextBox result = new() { IsReadOnly = true, AcceptsReturn = true,
        VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
        HorizontalScrollBarVisibility = ScrollBarVisibility.Auto,
        FontFamily = new FontFamily("Consolas"), MinHeight = 260,
        TextWrapping = TextWrapping.NoWrap };
    private readonly TextBlock status = new() { Text = "Gereed" };
    private readonly Button start = new() { Content = "Start speedtest", Padding = new Thickness(16, 9, 16, 9) };
    private readonly Button copy = new() { Content = "Kopieer JSON", IsEnabled = false };
    private readonly Button export = new() { Content = "Exporteer JSON", IsEnabled = false };
    private string? lastJson;

    internal SpeedtestWindow()
    {
        Title = customer ? "Onvi Speedtest" : "Qonnected NOC Speedtest";
        Width = customer ? 560 : 760;
        Height = customer ? 510 : 680;
        MinWidth = 460;
        MinHeight = 410;

        // The Onvi edition uses a separately deployed, locked configuration.
        // Do not silently use internal hostnames or guessed public endpoints.
        var settings = SpeedtestSettings.Load(customer);
        server.Text = settings.Server;
        port.Text = settings.ModernPort.ToString();
        protocol.Items.Add("Modern");
        protocol.Items.Add("Legacy 9");
        protocol.SelectedIndex = 0;
        protocol.SelectionChanged += (_, _) =>
        {
            if (!customer) port.Text = protocol.SelectedIndex == 1 ? "24602" : settings.ModernPort.ToString();
        };
        direction.Items.Add("Download");
        direction.Items.Add("Upload");
        direction.Items.Add("Download + Upload");
        direction.SelectedIndex = 2;

        var layout = new StackPanel { Margin = new Thickness(22) };
        var scroll = new ScrollViewer { Content = layout, VerticalScrollBarVisibility = ScrollBarVisibility.Auto };
        Content = scroll;
        var brand = LoadBrandImage(customer);
        if (brand is not null)
        {
            layout.Children.Add(new Image {
                Source = brand, Height = customer ? 74 : 66,
                Stretch = Stretch.Uniform, HorizontalAlignment = HorizontalAlignment.Left,
                Margin = new Thickness(0, 0, 0, 12)
            });
            Icon = brand;
        }
        layout.Children.Add(new TextBlock {
            Text = customer ? "Onvi internetsnelheid meten" : "Qonnected NOC — TR-471 Speedtest",
            FontSize = 23, FontWeight = FontWeights.SemiBold,
            Margin = new Thickness(0, 0, 0, 12)
        });
        if (!customer)
        {
            layout.Children.Add(Labeled("Server", server));
            layout.Children.Add(Labeled("Protocol", protocol));
            layout.Children.Add(Labeled("UDP-controlpoort", port));
            layout.Children.Add(Labeled("Richting", direction));
        }
        else
        {
            layout.Children.Add(new TextBlock {
                Text = "Klik op Start speedtest. De meetinstellingen zijn vooraf geconfigureerd.",
                TextWrapping = TextWrapping.Wrap,
                Margin = new Thickness(0, 0, 0, 14)
            });
        }
        start.Margin = new Thickness(0, 8, 0, 8);
        start.Click += async (_, _) => await RunTestAsync(settings);
        layout.Children.Add(start);
        layout.Children.Add(status);
        layout.Children.Add(result);
        if (!customer)
        {
            var actions = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 10, 0, 0) };
            copy.Margin = new Thickness(0, 0, 10, 0);
            copy.Click += (_, _) => { if (lastJson is not null) Clipboard.SetText(lastJson); };
            export.Click += (_, _) => ExportJson();
            actions.Children.Add(copy);
            actions.Children.Add(export);
            layout.Children.Add(actions);
        }
    }

    private static BitmapImage? LoadBrandImage(bool onvi)
    {
        // Original supplied brand assets; copied into the publish output by MSBuild.
        var file = Path.Combine(AppContext.BaseDirectory, "branding",
            onvi ? "Blauw.png" : "Qonnected-Kleur.png");
        if (!File.Exists(file)) return null;
        var bitmap = new BitmapImage();
        bitmap.BeginInit();
        bitmap.UriSource = new Uri(file, UriKind.Absolute);
        bitmap.CacheOption = BitmapCacheOption.OnLoad;
        bitmap.EndInit();
        bitmap.Freeze();
        return bitmap;
    }

    private static FrameworkElement Labeled(string name, UIElement input)
    {
        var panel = new StackPanel { Margin = new Thickness(0, 0, 0, 10) };
        panel.Children.Add(new TextBlock { Text = name, Margin = new Thickness(0, 0, 0, 4) });
        panel.Children.Add(input);
        return panel;
    }

    private async Task RunTestAsync(SpeedtestSettings settings)
    {
        if (customer && !settings.CustomerConfigured)
        {
            MessageBox.Show("Deze Onvi-build is nog niet voorzien van de publieke meetserverconfiguratie.",
                "Configuratie ontbreekt", MessageBoxButton.OK, MessageBoxImage.Warning);
            return;
        }
        if (!int.TryParse(port.Text, out var selectedPort) || selectedPort < 1 || selectedPort > 65535)
        {
            MessageBox.Show("Ongeldige UDP-poort.");
            return;
        }
        var selectedProtocol = customer ? "Modern" : protocol.SelectedItem?.ToString() ?? "Modern";
        var testPort = customer ? settings.ModernPort : selectedPort;
        var testHost = customer ? settings.Server : server.Text.Trim();
        var directions = customer || direction.SelectedIndex == 2
            ? new[] { "-d", "-u" }
            : new[] { direction.SelectedIndex == 0 ? "-d" : "-u" };

        start.IsEnabled = false;
        copy.IsEnabled = false;
        export.IsEnabled = false;
        lastJson = null;
        result.Text = "";
        try
        {
            var executable = EmbeddedEngine.Resolve();
            var reports = new List<JsonElement>();
            foreach (var option in directions)
            {
                status.Text = option == "-d" ? "Downloadmeting..." : "Uploadmeting...";
                var raw = await EngineRunner.RunAsync(executable, testHost, testPort, option, selectedProtocol);
                using var parsed = JsonDocument.Parse(raw);
                reports.Add(parsed.RootElement.Clone());
            }
            lastJson = JsonSerializer.Serialize(new {
                schema_version = "1.0",
                application = customer ? "onvi-speedtest" : "qonnected-noc-speedtest",
                measured_at_utc = DateTimeOffset.UtcNow,
                server = testHost,
                control_port = testPort,
                engine = selectedProtocol,
                results = reports
            }, new JsonSerializerOptions { WriteIndented = true });
            result.Text = customer ? BuildCustomerSummary(reports) : lastJson;
            status.Text = "Meting voltooid";
            copy.IsEnabled = !customer;
            export.IsEnabled = !customer;
        }
        catch (Exception ex) when (ex is IOException or InvalidOperationException or
                                   System.ComponentModel.Win32Exception or JsonException or
                                   OperationCanceledException)
        {
            status.Text = "Meting mislukt";
            result.Text = ex.Message;
        }
        finally { start.IsEnabled = true; }
    }

    private static string BuildCustomerSummary(IReadOnlyList<JsonElement> reports)
    {
        // Do not guess values from different UDPST protocol versions.
        // A validated metric mapping is needed before showing numeric speeds.
        return "De meetengine heeft de test afgerond.\n\n" +
               "De resultaten zijn nog niet als downloadsnelheid en uploadsnelheid weergegeven, " +
               "omdat de JSON-velden eerst tegen de echte Windows-meetengine moeten worden gevalideerd.\n\n" +
               "Neem bij vragen contact op met Onvi Support.";
    }

    private void ExportJson()
    {
        if (lastJson is null) return;
        var dialog = new Microsoft.Win32.SaveFileDialog {
            Filter = "JSON (*.json)|*.json",
            DefaultExt = ".json",
            FileName = "qonnected-speedtest-" + DateTime.Now.ToString("yyyyMMdd-HHmmss") + ".json"
        };
        if (dialog.ShowDialog(this) == true) File.WriteAllText(dialog.FileName, lastJson, Encoding.UTF8);
    }
}

internal sealed record SpeedtestSettings(string Server, int ModernPort,
    string EngineExecutable, bool CustomerConfigured)
{
    internal static SpeedtestSettings Load(bool customer)
    {
        var folder = AppContext.BaseDirectory;
        if (!customer)
            return new("speedtest.nikhef.qonnected.net", 24601,
                Path.Combine(folder, "udpst.exe"), true);

        var path = Path.Combine(folder, "onvi-settings.json");
        if (!File.Exists(path))
            return new("", 24601, Path.Combine(folder, "udpst.exe"), false);
        try
        {
            using var json = JsonDocument.Parse(File.ReadAllText(path));
            var root = json.RootElement;
            var host = root.GetProperty("server").GetString() ?? "";
            var p = root.GetProperty("port").GetInt32();
            var valid = !string.IsNullOrWhiteSpace(host) && p is >= 1 and <= 65535;
            return new(host, p, Path.Combine(folder, "udpst.exe"), valid);
        }
        catch (Exception ex) when (ex is IOException or JsonException or KeyNotFoundException
                                   or InvalidOperationException)
        {
            return new("", 24601, Path.Combine(folder, "udpst.exe"), false);
        }
    }
}

internal static class EmbeddedEngine
{
    private const string ResourceName = "Qonnected.Speedtest.Native.udpst.exe";

    internal static string Resolve()
    {
        using var resource = Assembly.GetExecutingAssembly().GetManifestResourceStream(ResourceName);
        if (resource is null)
            throw new IOException("Ingebouwde TR-471 Windows-meetengine ontbreekt in deze build.");

        using var buffer = new MemoryStream();
        resource.CopyTo(buffer);
        var bytes = buffer.ToArray();
        var digest = Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant();
        var directory = Path.Combine(Path.GetTempPath(), "Qonnected-Speedtest", digest);
        Directory.CreateDirectory(directory);
        var executable = Path.Combine(directory, "udpst.exe");
        if (!File.Exists(executable) || !SHA256.HashData(File.ReadAllBytes(executable)).SequenceEqual(SHA256.HashData(bytes)))
        {
            var staging = Path.Combine(directory, Guid.NewGuid().ToString("N") + ".tmp");
            File.WriteAllBytes(staging, bytes);
            try { File.Move(staging, executable, overwrite: true); }
            finally { if (File.Exists(staging)) File.Delete(staging); }
        }
        return executable;
    }
}

internal static class EngineRunner
{
    internal static async Task<string> RunAsync(string exe, string host, int port, string direction, string protocol)
    {
        if (!File.Exists(exe)) throw new IOException(
            "TR-471 Windows-meetengine ontbreekt: " + Path.GetFileName(exe));
        if (string.IsNullOrWhiteSpace(host)) throw new InvalidOperationException("Server ontbreekt.");
        using var process = new Process();
        process.StartInfo = new ProcessStartInfo {
            FileName = exe,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = true
        };
        // Native OB-UDPST uses short options; --protocol is not implemented.
        // A legacy server port alone does not guarantee legacy protocol negotiation.
        if (protocol != "Modern")
            throw new InvalidOperationException("Legacy protocol 8/9 is nog niet ondersteund door deze Windows-meetengine.");
        foreach (var arg in new[] { "-f", "json", "-p", port.ToString(), direction, host })
            process.StartInfo.ArgumentList.Add(arg);
        if (!process.Start()) throw new InvalidOperationException("Kan meetengine niet starten.");
        using var timeout = new CancellationTokenSource(TimeSpan.FromMinutes(5));
        try
        {
            var stdout = process.StandardOutput.ReadToEndAsync(timeout.Token);
            var stderr = process.StandardError.ReadToEndAsync(timeout.Token);
            await process.WaitForExitAsync(timeout.Token);
            var output = await stdout;
            var errors = await stderr;
            if (process.ExitCode != 0)
                throw new InvalidOperationException("Meetengine exitcode " + process.ExitCode + "\n\nStderr:\n" + (string.IsNullOrWhiteSpace(errors) ? "(leeg)" : errors) + "\n\nStdout:\n" + (string.IsNullOrWhiteSpace(output) ? "(leeg)" : output));
            return output;
        }
        catch (OperationCanceledException)
        {
            if (!process.HasExited) process.Kill(entireProcessTree: true);
            throw;
        }
    }
}
