using Fantome.Libraries.League.IO.WAD;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace modadd
{
    public class LeagueIndex
    {
        Dictionary<ulong, List<string>> _lookup;
        static LeagueIndex FromPathOrConfig(string path, string config)
        {

            return new LeagueIndex(path);
        }

        private LeagueIndex(string path)
        {
            foreach (var file in Directory.EnumerateFiles($"{path}/Game/DATA/FINAL", "*.wad.client", SearchOption.AllDirectories))
            {
                var wad = new WADFile(file);
                var name = Path.GetFileName(file);
                foreach (var hash in wad.Entries.Select(e => e.XXHash))
                {
                    if(!_lookup.ContainsKey(hash))
                    {
                        _lookup[hash] = new List<string>();
                    }
                    _lookup[hash].Add(name);
                }
            }
        }

        public string FindName(string path)
        {
            var found = new Dictionary<string, int>();
            var wad = new WADFile(path);

            foreach (var hash in wad.Entries.Select(e => e.XXHash))
            {
                if (_lookup.ContainsKey(hash))
                {
                    foreach (var name in _lookup[hash])
                    {
                        if (!found.ContainsKey(name))
                        {
                            found[name] = 1;
                        }
                        else
                        {
                            found[name] += 1;
                        }
                    }
                }
            }

            if (found.Count == 0)
            {
                return "Bootstrap.wad.client";
            }
            return found.OrderByDescending(x => x.Value).First().Key;
        }
    }
}
