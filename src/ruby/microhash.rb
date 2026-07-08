# frozen_string_literal: true

# MicroHash — lightweight, non-cryptographic 64-bit hash.
# Pure Ruby, no dependencies. Matches the C++/C# reference output.
module MicroHash
  MASK32 = 0xFFFFFFFF
  BLOCK_SIZE = 32

  module_function

  # Accepts a String (hashed as raw bytes, encoding-agnostic) or an
  # Array of byte values. Returns the 64-bit digest as an Integer.
  def compute_hash(data)
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

      4.times do |i|
        base = i * 4
        word = block[base] |
               (block[base + 1] << 8) |
               (block[base + 2] << 16) |
               (block[base + 3] << 24)

        s0 = (rol32(s0 ^ word, 5) + s1) & MASK32
        s1 = rol32((s1 + word) & MASK32, 11) ^ s0
      end
    end

    final = s0 ^ rol32(s1, 3)
    (final << 32) | s1
  end

  # 16-character uppercase hex digest, e.g. "352256EFEDC72BD1".
  def hexdigest(data)
    format('%016X', compute_hash(data))
  end

  def rol32(value, bits)
    ((value << bits) | (value >> (32 - bits))) & MASK32
  end
end
