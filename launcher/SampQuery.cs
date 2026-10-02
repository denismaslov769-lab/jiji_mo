using System.Net;
using System.Net.Sockets;
using System.Text;

namespace GodjoLauncher;

public sealed record ServerInfo(bool Online, int Players, int MaxPlayers, string Hostname, int Ping);

/// <summary>Запрос информации о сервере по протоколу SA-MP query (UDP, пакет 'i').</summary>
public static class SampQuery
{
    public static async Task<ServerInfo> QueryAsync(string host, int port, int timeoutMs = 2500)
    {
        try
        {
            var addrs = await Dns.GetHostAddressesAsync(host);
            var ip = addrs.First(a => a.AddressFamily == AddressFamily.InterNetwork);
            using var udp = new UdpClient();
            udp.Connect(ip, port);
            var b = ip.GetAddressBytes();
            var pkt = new byte[11];
            Encoding.ASCII.GetBytes("SAMP").CopyTo(pkt, 0);
            b.CopyTo(pkt, 4);
            pkt[8] = (byte)(port & 0xFF); pkt[9] = (byte)(port >> 8); pkt[10] = (byte)'i';
            var sw = System.Diagnostics.Stopwatch.StartNew();
            await udp.SendAsync(pkt, pkt.Length);
            var recv = udp.ReceiveAsync();
            if (await Task.WhenAny(recv, Task.Delay(timeoutMs)) != recv) return new(false, 0, 0, "", 0);
            var d = recv.Result.Buffer;
            int ping = (int)sw.ElapsedMilliseconds;
            int o = 11;
            o += 1; // passworded
            int players = BitConverter.ToUInt16(d, o); o += 2;
            int max = BitConverter.ToUInt16(d, o); o += 2;
            int hl = BitConverter.ToInt32(d, o); o += 4;
            var name = Encoding.GetEncoding(1251).GetString(d, o, Math.Min(hl, d.Length - o));
            return new(true, players, max, name, ping);
        }
        catch { return new(false, 0, 0, "", 0); }
    }
}
