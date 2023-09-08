local zstd = require "zstd"

local test_str = ""

for i = 1, 100000 do
    test_str = test_str .. i
end

local c_ctx = zstd.create_c_context()
local compress_str = c_ctx:compress(test_str, zstd.E_FLUSH)
local d_ctx = zstd.create_d_context()
local decompress_str = d_ctx:decompress(compress_str)

print(decompress_str == test_str)