# frozen_string_literal: true

# MicroHashNG CLI — mirrors the original Ruby tool.
#
#   ruby src/microhash-ng/ruby/main.rb "Hello, World!"
#   ruby src/microhash-ng/ruby/main.rb --test
#   ruby src/microhash-ng/ruby/main.rb            # interactive prompt

require_relative 'microhash_ng'

TEST_VECTORS = {
  'Hello, World!' => 0xA40E5C7D0BFBA07D,
  'The quick brown fox jumps over the lazy dog' => 0x5BD8C52E8C1E2175,
  '' => 0x6CA97D4E1A59E8EC,
  'a' => 0xD1EF310FB09DC1DC,
  'abc' => 0x1351FEBF7FEDB189,
  '        ' => 0x38AFC965BDFDC9EB,
  'abcdefghijklmnopqrstuvwxyz' => 0xF2A991C82844982F,
  'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789' => 0x7AB7F4398A2A0130,
  '0' * 64 => 0xBC55379EAAB952BF,
  '1' * 64 => 0x42AF241530C58F18,
  '123456789012345678901234567890' => 0x7A78EB4902E77E91,
  '01' * 32 => 0x38232B18B8755FA6,
  '0101011101010111010101010101011101010111000101010001110101010100' => 0x29E828BAC44A055B
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
