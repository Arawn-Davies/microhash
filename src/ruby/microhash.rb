# frozen_string_literal: true

# MicroHash — lightweight, non-cryptographic 64-bit hash.
# Matches the C++/C# reference output.
#
# If the native extension (ext/microhash) has been compiled, hashing is
# delegated to C for OpenSSL-digest-like speed; otherwise the pure-Ruby
# implementation below is used. Both produce identical output.
module MicroHash
  MASK32 = 0xFFFFFFFF
  BLOCK_SIZE = 32

  NATIVE = begin
    require_relative 'ext/microhash/microhash_ext'
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

    padded_len = ((length + 5 + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE
    padded = str.b << 0x80.chr << ("\0" * (padded_len - length - 5)) << [length].pack('N')
    words = padded.unpack('V*')

    s0 = 0x243F6A88
    s1 = 0x85A308D3

    # Original algorithm: only the first four words (bytes 0-15) of each
    # eight-word block are mixed.
    i = 0
    n = words.length
    while i < n
      j = i
      last = i + 4
      while j < last
        w = words[j]
        t = s0 ^ w
        s0 = ((((t << 5) | (t >> 27)) & MASK32) + s1) & MASK32
        t = (s1 + w) & MASK32
        s1 = (((t << 11) | (t >> 21)) & MASK32) ^ s0
        j += 1
      end
      i += 8
    end

    final = s0 ^ (((s1 << 3) | (s1 >> 29)) & MASK32)
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
