using System;
using System.Collections.Generic;
using System.Text;
using microhash_ng;

class Program
{
    static readonly (string Input, ulong Expected)[] TestVectors =
    {
        ("Hello, World!",                                                    0x3BC7B2EA7D9D9143ul),
        ("The quick brown fox jumps over the lazy dog",                      0x0BC09723C9A7F509ul),
        ("",                                                                 0x40D6DE95FA68D791ul),
        ("a",                                                                0xD04B9EC77726AB0Ful),
        ("abc",                                                              0x8D4B24AB0DD63EDBul),
        ("        ",                                                         0xCF7285AB13D90778ul),
        ("abcdefghijklmnopqrstuvwxyz",                                       0x2008399202128668ul),
        ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",   0x1662B9BA9DAC92CDul),
        ("0000000000000000000000000000000000000000000000000000000000000000", 0x81E4F9F09184C9CAul),
        ("1111111111111111111111111111111111111111111111111111111111111111", 0xA5046C4D639E45C6ul),
        ("123456789012345678901234567890",                                   0x96F4DBA8A6596732ul),
        ("0101010101010101010101010101010101010101010101010101010101010101", 0x9FEACB10CEA370AEul),
        ("0101011101010111010101010101011101010111000101010001110101010100", 0x38F625D205173523ul),
    };

    static ulong Hash(string s) => hashPipeNG.ComputeHash(Encoding.UTF8.GetBytes(s));

    static int RunTests()
    {
        int failures = 0;
        foreach (var (input, expected) in TestVectors)
        {
            ulong actual = Hash(input);
            bool pass = actual == expected;
            if (!pass) failures++;
            Console.WriteLine($"[{(pass ? "PASS" : "FAIL")}] microhash-ng(\"{input}\") = 0x{actual:X16}");
        }
        Console.WriteLine(failures == 0 ? "All tests passed." : $"{failures} test(s) FAILED.");
        return failures == 0 ? 0 : 1;
    }

    static int Main(string[] args)
    {
        if (args.Length > 0 && args[0] == "--test")
            return RunTests();

        string input;
        if (args.Length > 0)
        {
            input = string.Join(' ', args);
        }
        else
        {
            Console.WriteLine("Enter a string to hash using microhash-ng:");
            Console.Write("> ");
            input = Console.ReadLine() ?? "";
        }

        Console.WriteLine($"microhash-ng(\"{input}\") = 0x{Hash(input):X16}");
        return 0;
    }
}
