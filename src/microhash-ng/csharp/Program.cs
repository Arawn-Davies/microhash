using System;
using System.Collections.Generic;
using System.Text;
using microhash_ng;

class Program
{
    static readonly (string Input, ulong Expected)[] TestVectors =
    {
        ("Hello, World!",                                                    0xA40E5C7D0BFBA07Dul),
        ("The quick brown fox jumps over the lazy dog",                      0x5BD8C52E8C1E2175ul),
        ("",                                                                 0x6CA97D4E1A59E8ECul),
        ("a",                                                                0xD1EF310FB09DC1DCul),
        ("abc",                                                              0x1351FEBF7FEDB189ul),
        ("        ",                                                         0x38AFC965BDFDC9EBul),
        ("abcdefghijklmnopqrstuvwxyz",                                       0xF2A991C82844982Ful),
        ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",   0x7AB7F4398A2A0130ul),
        ("0000000000000000000000000000000000000000000000000000000000000000", 0xBC55379EAAB952BFul),
        ("1111111111111111111111111111111111111111111111111111111111111111", 0x42AF241530C58F18ul),
        ("123456789012345678901234567890",                                   0x7A78EB4902E77E91ul),
        ("0101010101010101010101010101010101010101010101010101010101010101", 0x38232B18B8755FA6ul),
        ("0101011101010111010101010101011101010111000101010001110101010100", 0x29E828BAC44A055Bul),
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
