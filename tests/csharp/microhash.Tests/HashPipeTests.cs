using System.Text;
using microhash;

namespace microhash.Tests;

/// <summary>
/// Test suite for the MicroHash C# implementation.
/// Mirrors the benchmark inputs from Benchmarks.cs (small / medium / large)
/// and covers the same test vectors used in the CLI's --test mode.
/// </summary>
public class HashPipeTests
{
    // -----------------------------------------------------------------------
    // Test vectors  (golden values validated against the reference impl)
    // -----------------------------------------------------------------------

    public static IEnumerable<object[]> TestVectorData =>
    [
        ["Hello, World!",                                                               0x352256EFEDC72BD1UL],
        ["The quick brown fox jumps over the lazy dog",                                 0x37876396F9CCB637UL],
        ["",                                                                            0xFD1FADBB7E12CB96UL],
        ["a",                                                                           0x9B1F9089AF49253EUL],
        ["abc",                                                                         0x8874CA7BE18B8218UL],
        ["        ",                                                                    0xB94BF2A5D5341A60UL],
        ["abcdefghijklmnopqrstuvwxyz",                                                  0x67773BF7A225BE5DUL],
        ["ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",             0xCE821AC98900EEA0UL],
        ["0000000000000000000000000000000000000000000000000000000000000000",             0x3411C1C38205A8E0UL],
        ["1111111111111111111111111111111111111111111111111111111111111111",             0x067FE50AF384C88EUL],
        ["123456789012345678901234567890",                                               0xBFD7E4924ACFA323UL],
        ["0101010101010101010101010101010101010101010101010101010101010101",             0x7BCC8A21375360E0UL],
        ["0101011101010111010101010101011101010111000101010001110101010100",             0xFDFB4707123CF187UL],
    ];

    [Theory]
    [MemberData(nameof(TestVectorData))]
    public void ComputeHash_KnownVectors_ReturnsExpected(string input, ulong expected)
    {
        byte[] data = Encoding.UTF8.GetBytes(input);
        ulong actual = hashPipe.ComputeHash(data);
        Assert.Equal(expected, actual);
    }

    // -----------------------------------------------------------------------
    // Benchmark-style inputs (mirrors Benchmarks.cs Setup())
    // -----------------------------------------------------------------------

    [Fact]
    public void ComputeHash_SmallInput_Veni()
    {
        // Small: "veni" (4 bytes) — first word of 'Veni vidi vici'
        byte[] data = Encoding.UTF8.GetBytes("veni");
        ulong actual = hashPipe.ComputeHash(data);
        Assert.Equal(0x8CF2BA1EF820120BUL, actual);
    }

    [Fact]
    public void ComputeHash_MediumInput_1KB()
    {
        // Medium: 1 KB filled with i % 256
        byte[] data = new byte[1024];
        for (int i = 0; i < data.Length; i++) data[i] = (byte)(i % 256);
        ulong actual = hashPipe.ComputeHash(data);
        Assert.Equal(0x0B1E05784B4A80E1UL, actual);
    }

    [Fact]
    public void ComputeHash_LargeInput_1MB()
    {
        // Large: 1 MB filled with i % 256
        byte[] data = new byte[1024 * 1024];
        for (int i = 0; i < data.Length; i++) data[i] = (byte)(i % 256);
        ulong actual = hashPipe.ComputeHash(data);
        Assert.Equal(0xD1F95722CE050E61UL, actual);
    }

    // -----------------------------------------------------------------------
    // Determinism: identical inputs always produce the same hash
    // -----------------------------------------------------------------------

    [Theory]
    [InlineData("Hello, World!")]
    [InlineData("")]
    [InlineData("abc")]
    [InlineData("veni")]
    public void ComputeHash_SameInput_ReturnsSameHash(string input)
    {
        byte[] data = Encoding.UTF8.GetBytes(input);
        ulong h1 = hashPipe.ComputeHash(data);
        ulong h2 = hashPipe.ComputeHash(data);
        Assert.Equal(h1, h2);
    }

    // -----------------------------------------------------------------------
    // Sensitivity: different inputs produce different hashes
    // -----------------------------------------------------------------------

    [Theory]
    [InlineData("a",            "b")]
    [InlineData("abc",          "ABC")]
    [InlineData("Hello, World!", "hello, world!")]
    [InlineData("ab",           "ba")]
    [InlineData("aaaa",         "aaab")]
    [InlineData("",             " ")]
    [InlineData("",             "a")]
    [InlineData("test",         "test!")]
    public void ComputeHash_DifferentInputs_ReturnDifferentHashes(string a, string b)
    {
        ulong ha = hashPipe.ComputeHash(Encoding.UTF8.GetBytes(a));
        ulong hb = hashPipe.ComputeHash(Encoding.UTF8.GetBytes(b));
        Assert.NotEqual(ha, hb);
    }

    // -----------------------------------------------------------------------
    // Edge cases
    // -----------------------------------------------------------------------

    [Fact]
    public void ComputeHash_EmptyArray_ReturnsKnownValue()
    {
        ulong actual = hashPipe.ComputeHash([]);
        Assert.Equal(0xFD1FADBB7E12CB96UL, actual);
    }

    [Fact]
    public void ComputeHash_AllSingleByteValues_DifferFromEmpty()
    {
        // Every single-byte value should produce a hash different from empty
        ulong emptyHash = hashPipe.ComputeHash([]);
        for (int b = 0; b < 256; b++)
        {
            ulong h = hashPipe.ComputeHash([(byte)b]);
            Assert.NotEqual(emptyHash, h);
        }
    }

    [Theory]
    [InlineData(31)]
    [InlineData(32)]
    [InlineData(33)]
    public void ComputeHash_BlockBoundaryLengths_ProduceDistinctHashes(int length)
    {
        // Strings of lengths around the 32-byte block boundary must differ
        byte[] a = Encoding.UTF8.GetBytes(new string('x', length));
        byte[] b = Encoding.UTF8.GetBytes(new string('x', length + 1));
        Assert.NotEqual(hashPipe.ComputeHash(a), hashPipe.ComputeHash(b));
    }
}
