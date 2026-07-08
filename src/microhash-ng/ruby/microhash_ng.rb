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
  def pure_compute_hash(data)
    bytes = data.is_a?(String) ? data.bytes : data
    length = bytes.length

    s0 = 0x243F6A88
    s1 = 0x85A308D3

    padded_len = ((length + 5 + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE

    (0...padded_len).step(BLOCK_SIZE) do |offset|
      block = Array.new(BLOCK_SIZE) do |i|
        index = offset + i
        if index < length
          bytes[index]
        elsif index == length
          0x80
        elsif index >= padded_len - 4
          (length >> (8 * (BLOCK_SIZE - 1 - i))) & 0xFF
        else
          0x00
        end
      end

      # ng: all eight words per block, two ARX rounds per word
      8.times do |i|
        base = i * 4
        word = block[base] |
               (block[base + 1] << 8) |
               (block[base + 2] << 16) |
               (block[base + 3] << 24)

        s0 = (rol32(s0 ^ word, 5) + s1) & MASK32
        s1 = rol32((s1 + word) & MASK32, 11) ^ s0
        s0 = (rol32(s0 ^ rol32(word, 16), 5) + s1) & MASK32
        s1 = rol32((s1 + word) & MASK32, 11) ^ s0
      end
    end

    FINAL_CONSTANTS.each do |c|
      s0 = (rol32(s0 ^ c, 5) + s1) & MASK32
      s1 = rol32((s1 + c) & MASK32, 11) ^ s0
    end

    final = s0 ^ rol32(s1, 3)
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
