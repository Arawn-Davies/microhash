# frozen_string_literal: true

# MicroHashNG — revised microhash, hardened against the SMHasher battery.
#
# Differences from the original (src/ruby/microhash.rb):
#   - All eight 4-byte words of each 32-byte block are mixed (no dead zone;
#     the encoded length field influences the digest).
#   - Each word is absorbed with TWO ARX rounds (the second reinjects the
#     word rotated by 16), preventing the sparse-key collision cancellations
#     SMHasher found in single-round absorption.
#   - Four ARX finalization rounds with pi-derived constants diffuse the
#     final words into the whole state.
#
# Output is NOT compatible with original microhash.
#
# If the native extension (ext/microhash_ng) has been compiled, hashing is
# delegated to C; otherwise the pure-Ruby implementation below is used.
# Both produce identical output.
module MicroHashNG
  MASK32 = 0xFFFFFFFF
  BLOCK_SIZE = 32
  FINAL_CONSTANTS = [0x13198A2E, 0x03707344, 0xA4093822, 0x299F31D0].freeze

  NATIVE = begin
    require_relative 'ext/microhash_ng/microhash_ng_ext'
    true
  rescue LoadError
    false
  end

  module_function

  # Accepts a String (hashed as raw bytes, encoding-agnostic) or an
  # Array of byte values. Returns the 64-bit digest as an Integer.
  def compute_hash(data)
    if NATIVE
      return native_compute_hash(data.is_a?(String) ? data : data.pack('C*'))
    end

    pure_compute_hash(data)
  end

  # Pure-Ruby reference path; used when the native extension is absent.
  #
  # Performance notes: the whole padded message is built once as a binary
  # String and split into little-endian words by String#unpack ('V*'), which
  # runs in C. The mixing loop inlines the rotates instead of calling rol32
  # per word. Output is identical to the native extension and the C++/C#
  # implementations.
  def pure_compute_hash(data)
    str = data.is_a?(String) ? data : data.pack('C*')
    length = str.bytesize

    # Padding: 0x80, zero fill, then big-endian 32-bit length in the last
    # 4 bytes — the layout shared by every implementation.
    padded_len = ((length + 5 + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE
    padded = str.b << 0x80.chr << ("\0" * (padded_len - length - 5)) << [length].pack('N')
    words = padded.unpack('V*')

    s0 = 0x243F6A88
    s1 = 0x85A308D3

    # ng: every word absorbed with two ARX rounds (second reinjects the
    # word rotated left by 16)
    i = 0
    n = words.length
    while i < n
      w = words[i]
      t = s0 ^ w
      s0 = ((((t << 5) | (t >> 27)) & MASK32) + s1) & MASK32
      t = (s1 + w) & MASK32
      s1 = (((t << 11) | (t >> 21)) & MASK32) ^ s0
      t = s0 ^ (((w << 16) | (w >> 16)) & MASK32)
      s0 = ((((t << 5) | (t >> 27)) & MASK32) + s1) & MASK32
      t = (s1 + w) & MASK32
      s1 = (((t << 11) | (t >> 21)) & MASK32) ^ s0
      i += 1
    end

    FINAL_CONSTANTS.each do |c|
      t = s0 ^ c
      s0 = ((((t << 5) | (t >> 27)) & MASK32) + s1) & MASK32
      t = (s1 + c) & MASK32
      s1 = (((t << 11) | (t >> 21)) & MASK32) ^ s0
    end

    final = s0 ^ (((s1 << 3) | (s1 >> 29)) & MASK32)
    (final << 32) | s1
  end

  # 16-character uppercase hex digest.
  def hexdigest(data)
    format('%016X', compute_hash(data))
  end

  def rol32(value, bits)
    ((value << bits) | (value >> (32 - bits))) & MASK32
  end
end
