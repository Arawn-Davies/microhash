# frozen_string_literal: true

require 'mkmf'

append_cflags('-O2')
create_makefile('microhash_ext')
