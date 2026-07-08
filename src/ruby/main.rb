# frozen_string_literal: true

# MicroHash CLI — mirrors the C++/C# tools.
#
#   ruby src/ruby/main.rb "Hello, World!"   # hash a string
#   ruby src/ruby/main.rb The quick brown   # words joined with spaces
#   ruby src/ruby/main.rb --test            # run built-in test vectors
#   ruby src/ruby/main.rb                   # interactive prompt

require_relative 'microhash'

TEST_VECTORS = {
  'Hello, World!' => 0x352256EFEDC72BD1,
  'The quick brown fox jumps over the lazy dog' => 0x37876396F9CCB637,
  '' => 0xFD1FADBB7E12CB96,
  'a' => 0x9B1F9089AF49253E,
  'abc' => 0x8874CA7BE18B8218,
  '        ' => 0xB94BF2A5D5341A60,
  'abcdefghijklmnopqrstuvwxyz' => 0x67773BF7A225BE5D,
  'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789' => 0xCE821AC98900EEA0,
  '0' * 64 => 0x3411C1C38205A8E0,
  '1' * 64 => 0x067FE50AF384C88E,
  '123456789012345678901234567890' => 0xBFD7E4924ACFA323,
  '01' * 32 => 0x7BCC8A21375360E0,
  '0101011101010111010101010101011101010111000101010001110101010100' => 0xFDFB4707123CF187
}.freeze

def run_tests
  failures = 0
  TEST_VECTORS.each do |input, expected|
    actual = MicroHash.compute_hash(input)
    status = actual == expected ? 'PASS' : 'FAIL'
    failures += 1 unless actual == expected
    printf("[%s] microhash(%-64p) = 0x%016X\n", status, input, actual)
  end
  puts failures.zero? ? 'All tests passed.' : "#{failures} test(s) FAILED."
  exit(failures.zero? ? 0 : 1)
end

if ARGV.first == '--test'
  run_tests
elsif ARGV.any?
  input = ARGV.join(' ')
  puts "0x#{MicroHash.hexdigest(input)}"
else
  print 'Enter text to hash: '
  input = $stdin.gets&.chomp || ''
  puts "0x#{MicroHash.hexdigest(input)}"
end
