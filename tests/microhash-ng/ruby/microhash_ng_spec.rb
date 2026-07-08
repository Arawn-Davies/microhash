# frozen_string_literal: true

# RSpec suite for the MicroHashNG Ruby implementation.
# Golden vectors plus regression tests for the two defects fixed relative
# to original microhash: the byte-16-31 dead zone and the unread length field.
#
#   rspec tests/microhash-ng/ruby/microhash_ng_spec.rb

require_relative '../../../src/microhash-ng/ruby/microhash_ng'

RSpec.describe MicroHashNG do
  TEST_VECTORS = {
    'Hello, World!' => 0x3BC7B2EA7D9D9143,
    'The quick brown fox jumps over the lazy dog' => 0x0BC09723C9A7F509,
    '' => 0x40D6DE95FA68D791,
    'a' => 0xD04B9EC77726AB0F,
    'abc' => 0x8D4B24AB0DD63EDB,
    '        ' => 0xCF7285AB13D90778,
    'abcdefghijklmnopqrstuvwxyz' => 0x2008399202128668,
    'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789' => 0x1662B9BA9DAC92CD,
    '0' * 64 => 0x81E4F9F09184C9CA,
    '1' * 64 => 0xA5046C4D639E45C6,
    '123456789012345678901234567890' => 0x96F4DBA8A6596732,
    '01' * 32 => 0x9FEACB10CEA370AE,
    '0101011101010111010101010101011101010111000101010001110101010100' => 0x38F625D205173523
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
  end

  describe 'regression: no dead zones (original ignored bytes 16-31 of each block)' do
    [32, 64, 256].each do |len|
      it "detects every single-byte flip in a #{len}-byte input" do
        base = Array.new(len) { |j| (j * 7) % 256 }
        ref = described_class.compute_hash(base)
        undetected = (0...len).select do |i|
          mod = base.dup
          mod[i] ^= 0xFF
          described_class.compute_hash(mod) == ref
        end
        expect(undetected).to be_empty
      end
    end

    it 'no longer collides on contents that agree only on bytes 0-15 of each block' do
      a = ('A' * 16) + ('X' * 16) + ('A' * 16) + ('Y' * 16)
      b = ('A' * 16) + ('Z' * 16) + ('A' * 16) + ('W' * 16)
      expect(described_class.compute_hash(a)).not_to eq(described_class.compute_hash(b))
    end
  end

  describe 'regression: length field is mixed (original never read it)' do
    it 'detects a trailing NUL append' do
      expect(described_class.compute_hash("abc\0"))
        .not_to eq(described_class.compute_hash('abc'))
    end

    it 'detects truncation within the same padding block' do
      expect(described_class.compute_hash('x' * 27))
        .not_to eq(described_class.compute_hash('x' * 26))
    end
  end

  describe '.pure_compute_hash' do
    it 'matches compute_hash for every test vector (native or not)' do
      TEST_VECTORS.each do |input, expected|
        expect(described_class.pure_compute_hash(input)).to eq(expected)
      end
    end

    if MicroHashNG::NATIVE
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
      expect(described_class.hexdigest('Hello, World!')).to eq('3BC7B2EA7D9D9143')
    end
  end
end
