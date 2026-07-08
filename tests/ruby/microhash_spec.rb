# frozen_string_literal: true

# RSpec suite for the MicroHash Ruby implementation.
# Covers the same golden test vectors as the C++ and C# suites.
#
#   rspec tests/ruby/microhash_spec.rb

require_relative '../../src/ruby/microhash'

RSpec.describe MicroHash do
  TEST_VECTORS = {
    'Hello, World!' => 0x352256EFEDC72BD1,
    'The quick brown fox jumps over the lazy dog' => 0x37876396F9CCB637,
    '' => 0xFD1FADBB7E12CB96,
    'a' => 0x9B1F9089AF49253E,
    'abc' => 0x8874CA7BE18B8218,
    '        ' => 0xB94BF2A5D5341A60,
    'abcdefghijklmnopqrstuvwxyz' => 0x67773BF7A225BE5D,
    'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789' => 0xCE821AC98900EEA0,
    '0000000000000000000000000000000000000000000000000000000000000000' => 0x3411C1C38205A8E0,
    '1111111111111111111111111111111111111111111111111111111111111111' => 0x067FE50AF384C88E,
    '123456789012345678901234567890' => 0xBFD7E4924ACFA323,
    '0101010101010101010101010101010101010101010101010101010101010101' => 0x7BCC8A21375360E0,
    '0101011101010111010101010101011101010111000101010001110101010100' => 0xFDFB4707123CF187
  }.freeze

  describe '.compute_hash' do
    TEST_VECTORS.each do |input, expected|
      it format('hashes %p to 0x%016X', input, expected) do
        expect(described_class.compute_hash(input)).to eq(expected)
      end
    end

    it 'accepts an array of byte values' do
      expect(described_class.compute_hash([0x61, 0x62, 0x63]))
        .to eq(described_class.compute_hash('abc'))
    end

    it 'is encoding-agnostic (hashes raw bytes)' do
      utf8   = 'héllo'.encode(Encoding::UTF_8)
      binary = utf8.dup.force_encoding(Encoding::BINARY)
      expect(described_class.compute_hash(utf8)).to eq(described_class.compute_hash(binary))
    end

    it 'always fits in 64 bits' do
      TEST_VECTORS.each_key do |input|
        expect(described_class.compute_hash(input)).to be_between(0, 0xFFFFFFFFFFFFFFFF)
      end
    end

    # Block size is 32 and padding adds a minimum of 5 bytes, so these
    # lengths exercise the single/multi-block and padding-overflow paths.
    it 'is deterministic across block-boundary lengths' do
      [26, 27, 28, 31, 32, 33, 59, 60, 63, 64, 65].each do |len|
        input = 'x' * len
        expect(described_class.compute_hash(input))
          .to eq(described_class.compute_hash(input.dup)), "length #{len}"
      end
    end
  end

  describe '.pure_compute_hash' do
    it 'matches compute_hash for every test vector (native or not)' do
      TEST_VECTORS.each do |input, expected|
        expect(described_class.pure_compute_hash(input)).to eq(expected)
      end
    end

    if MicroHash::NATIVE
      it 'agrees with the native extension across many lengths' do
        (0..200).each do |len|
          input = (0...len).map { |i| ((i * 7) + len) % 256 }.pack('C*')
          expect(described_class.native_compute_hash(input))
            .to eq(described_class.pure_compute_hash(input)), "length #{len}"
        end
      end
    end
  end

  describe '.hexdigest' do
    it 'returns a 16-character uppercase hex string' do
      expect(described_class.hexdigest('Hello, World!')).to eq('352256EFEDC72BD1')
    end

    it 'zero-pads small digests to 16 characters' do
      expect(described_class.hexdigest('').length).to eq(16)
    end
  end
end
