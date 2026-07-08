# frozen_string_literal: true

# MicroHashNG CLI — mirrors the original Ruby tool.
#
#   ruby src/microhash-ng/ruby/main.rb "Hello, World!"
#   ruby src/microhash-ng/ruby/main.rb --test
#   ruby src/microhash-ng/ruby/main.rb            # interactive prompt

require_relative 'microhash_ng'

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

def run_tests
  failures = 0
  TEST_VECTORS.each do |input, expected|
    actual = MicroHashNG.compute_hash(input)
    status = actual == expected ? 'PASS' : 'FAIL'
    failures += 1 unless actual == expected
    printf("[%s] microhash-ng(%-64p) = 0x%016X\n", status, input, actual)
  end
  puts failures.zero? ? 'All tests passed.' : "#{failures} test(s) FAILED."
  exit(failures.zero? ? 0 : 1)
end

if ARGV.first == '--test'
  run_tests
elsif ARGV.any?
  puts "0x#{MicroHashNG.hexdigest(ARGV.join(' '))}"
else
  print 'Enter text to hash: '
  input = $stdin.gets&.chomp || ''
  puts "0x#{MicroHashNG.hexdigest(input)}"
end
