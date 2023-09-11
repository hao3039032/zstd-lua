// zstd-lua
#include <cstdio>
#include <climits>
#include <cstring>
#include <zstd.h>
#include <vector>
#include <memory>

using namespace std;

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>

    #define check_c_context(L, idx)\
            *(ZSTD_CCtx **)luaL_checkudata(L, idx, "zstd_c_context")

    #define check_d_context(L, idx)\
            *(ZSTD_DCtx **)luaL_checkudata(L, idx, "zstd_d_context")

    #define check_c_dict(L, idx)\
            *(ZSTD_CDict **)luaL_checkudata(L, idx, "zstd_c_dict")

    #define check_d_dict(L, idx)\
            *(ZSTD_DDict **)luaL_checkudata(L, idx, "zstd_d_dict")

    /**
     * ZSTD D Dict
     * 
     */
    static int lua_zstd_d_dict_tostring(lua_State *L) {
        lua_settop(L, 1);
        ZSTD_DDict * d_dict = check_d_dict(L, 1);

        lua_settop(L, 0);

        lua_pushfstring(L, "zstd_d_dict: %p", d_dict);
        return 1;
    }

    static int lua_zstd_d_dict_gc(lua_State *L) {
        lua_settop(L, 1);
        ZSTD_DDict ** p_d_dict = (ZSTD_DDict **)luaL_checkudata(L, 1, "zstd_d_dict");
        ZSTD_DDict * d_dict = *p_d_dict;

        lua_settop(L, 0);

        ZSTD_freeDDict(d_dict);
        *p_d_dict = nullptr;
        return 0;
    }

    static int lua_zstd_create_d_dict(lua_State *L) {
        lua_settop(L, 1);
        size_t dict_buff_len;
        const char * dict_buff = luaL_checklstring(L, 1, &dict_buff_len);

        ZSTD_DDict * d_dict = ZSTD_createDDict(dict_buff, dict_buff_len);
        if (!d_dict) {
            return luaL_error(L, "zstd error: can not create a DDict");
        }

        ZSTD_DDict ** p_ddict = (ZSTD_DDict **)lua_newuserdata(L, sizeof(void *));

        lua_settop(L, 0);
        if (luaL_newmetatable(L, "zstd_d_dict")) {
            *p_ddict = d_dict;

            lua_pushcfunction(L, lua_zstd_d_dict_tostring);
            lua_setfield(L, -2, "__tostring");

            lua_pushcfunction(L, lua_zstd_d_dict_gc);
            lua_setfield(L, -2, "__gc");

            lua_setmetatable(L, -2);
            return 1;
        } else {
            ZSTD_freeDDict(d_dict);
            return luaL_error(L, "zstd error: can not create a zstd_d_dict metatable");
        }
    }


    /**
     * ZSTD C Dict
     * 
     */
    
    static int lua_zstd_c_dict_tostring(lua_State *L) {
        lua_settop(L, 1);
        ZSTD_CDict * c_dict = check_c_dict(L, 1);

        lua_settop(L, 0);

        lua_pushfstring(L, "zstd_c_dict: %p", c_dict);
        return 1;
    }

    static int lua_zstd_c_dict_gc(lua_State *L) {
        lua_settop(L, 1);
        ZSTD_CDict ** p_c_dict = (ZSTD_CDict **)luaL_checkudata(L, 1, "zstd_c_dict");
        ZSTD_CDict * c_dict = *p_c_dict;

        lua_settop(L, 0);

        ZSTD_freeCDict(c_dict);
        *p_c_dict = nullptr;
        return 0;
    }

    static int lua_zstd_create_c_dict(lua_State *L) {
        lua_settop(L, 2);
        size_t dict_buff_len;
        const char * dict_buff = luaL_checklstring(L, 1, &dict_buff_len);
        int level = luaL_optinteger(L, 2, ZSTD_CLEVEL_DEFAULT);

        ZSTD_CDict * c_dict = ZSTD_createCDict(dict_buff, dict_buff_len, level);
        if (!c_dict) {
            return luaL_error(L, "zstd error: can not create a CDict");
        }

        ZSTD_CDict ** p_cdict = (ZSTD_CDict **)lua_newuserdata(L, sizeof(void *));

        lua_settop(L, 0);
        if (luaL_newmetatable(L, "zstd_c_dict")) {
            *p_cdict = c_dict;

            lua_pushcfunction(L, lua_zstd_c_dict_tostring);
            lua_setfield(L, -2, "__tostring");

            lua_pushcfunction(L, lua_zstd_c_dict_gc);
            lua_setfield(L, -2, "__gc");

            lua_setmetatable(L, -2);
            return 1;
        } else {
            ZSTD_freeCDict(c_dict);
            return luaL_error(L, "zstd error: can not create a zstd_c_dict metatable");
        }
    }

    /**
     * ZSTD C Context
     * 
     */

    static int lua_zstd_c_context_compress(lua_State *L) {
        lua_settop(L, 3);
        ZSTD_CCtx * ctx = check_c_context(L, 1);
        size_t data_len;
        const char * data = luaL_checklstring(L, 2, &data_len);
        ZSTD_EndDirective mode = (ZSTD_EndDirective)luaL_checkinteger(L, 3);

        lua_settop(L, 0);

        if (!ctx) return luaL_error(L, "ctx has already release");

        if (mode != ZSTD_e_continue && mode != ZSTD_e_end && mode != ZSTD_e_flush) {
            return luaL_error(L, "end mode should be one of contine, end or flush");
        }

        ZSTD_inBuffer input {data, data_len, 0};

        size_t buffer_out_size = ZSTD_CStreamOutSize();
        shared_ptr<char[]> buffer_out(new char[buffer_out_size]);
        vector<char> result;

        ZSTD_outBuffer output {buffer_out.get(), buffer_out_size, 0};

        bool finished = false;
        size_t remaining = 0;

        do {
            output.pos = 0;
            remaining = ZSTD_compressStream2(ctx, &output, &input, mode);

            if (ZSTD_isError(remaining)) {
                return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(remaining));
            }

            if (output.pos > 0) copy(buffer_out.get(), buffer_out.get() + output.pos, back_inserter(result));
            finished = mode != ZSTD_e_continue ? (remaining == 0) : (input.pos == input.size);
        } while(!finished);

        lua_pushlstring(L, result.data(), result.size());
        return 1;
    }

    static int lua_zstd_c_context_set_pledged_src_size(lua_State *L) {
        lua_settop(L, 2);
        ZSTD_CCtx * ctx = check_c_context(L, 1);
        unsigned long long size = luaL_checkinteger(L, 2);

        lua_settop(L, 0);
        if (!ctx) return luaL_error(L, "ctx has already release");

        size_t ret = ZSTD_CCtx_setPledgedSrcSize(ctx, size);
        if (ZSTD_isError(ret)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(ret));
        }

        lua_pushinteger(L, ret);
        return 1;
    }

    static int lua_zstd_c_context_enable_checksum(lua_State *L) {
        lua_settop(L, 2);
        ZSTD_CCtx * ctx = check_c_context(L, 1);
        int enable = luaL_checkinteger(L, 2);

        lua_settop(L, 0);
        if (!ctx) return luaL_error(L, "ctx has already release");

        size_t ret = ZSTD_CCtx_setParameter(ctx, ZSTD_c_checksumFlag, enable);
        if (ZSTD_isError(ret)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(ret));
        }

        lua_pushinteger(L, ret);
        return 1;
    }

    static int lua_zstd_c_context_reset(lua_State *L) {
        lua_settop(L, 2);
        ZSTD_CCtx * ctx = check_c_context(L, 1);
        ZSTD_ResetDirective flag = (ZSTD_ResetDirective)luaL_checkinteger(L, 2);

        lua_settop(L, 0);
        if (!ctx) return luaL_error(L, "ctx has already release");

        size_t ret = ZSTD_CCtx_reset(ctx, flag);
        if (ZSTD_isError(ret)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(ret));
        }

        lua_pushinteger(L, ret);
        return 1;
    }

    static int lua_zstd_c_context_load_dict(lua_State *L) {
        lua_settop(L, 2);
        ZSTD_CCtx * ctx = check_c_context(L, 1);
        size_t dict_len;
        const char * dict = luaL_checklstring(L, 2, &dict_len);

        lua_settop(L, 0);
        if (!ctx) return luaL_error(L, "ctx has already release");

        size_t ret = ZSTD_CCtx_loadDictionary(ctx, dict, dict_len);

        if (ZSTD_isError(ret)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(ret));
        }

        lua_pushinteger(L, ret);
        return 1;
    }

    static int lua_zstd_c_context_clear_dict(lua_State *L) {
        lua_settop(L, 1);
        ZSTD_CCtx * ctx = check_c_context(L, 1);

        lua_settop(L, 0);
        if (!ctx) return luaL_error(L, "ctx has already release");

        size_t ret = ZSTD_CCtx_refCDict(ctx, nullptr);

        if (ZSTD_isError(ret)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(ret));
        }

        lua_pushinteger(L, ret);
        return 1;
    }

    static int lua_zstd_c_context_ref_dict(lua_State *L) {
        lua_settop(L, 2);
        ZSTD_CCtx * ctx = check_c_context(L, 1);
        ZSTD_CDict * cdict = check_c_dict(L, 2);

        lua_settop(L, 0);
        if (!ctx) return luaL_error(L, "ctx has already release");
        if (!cdict) return luaL_error(L, "cdict has already release");

        size_t ret = ZSTD_CCtx_refCDict(ctx, cdict);
        
        if (ZSTD_isError(ret)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(ret));
        }

        lua_pushinteger(L, ret);
        return 1;
    }

    static const luaL_Reg lua_zstd_c_context_functions[] = {
        { "compress", lua_zstd_c_context_compress },
        { "set_pledged_src_size", lua_zstd_c_context_set_pledged_src_size},
        { "enable_checksum", lua_zstd_c_context_enable_checksum },
        { "reset", lua_zstd_c_context_reset },
        { "load_dict", lua_zstd_c_context_load_dict },
        { "clear_dict", lua_zstd_c_context_clear_dict },
        { "ref_dict", lua_zstd_c_context_ref_dict },
        { nullptr, nullptr }
    };

    static int lua_zstd_c_context_tostring(lua_State *L)
    {
        lua_settop(L, 1);
        ZSTD_CCtx * c_context = check_c_context(L, 1);

        lua_settop(L, 0);

        lua_pushfstring(L, "zstd_c_context: %p", c_context);
        return 1;
    }

    static int lua_zstd_c_context_gc(lua_State *L)
    {
        lua_settop(L, 1);
        ZSTD_CCtx ** p_c_context = (ZSTD_CCtx **)luaL_checkudata(L, 1, "zstd_c_context");
        ZSTD_CCtx * c_context = *p_c_context;

        lua_settop(L, 0);

        ZSTD_freeCCtx(c_context);
        *p_c_context = nullptr;

        return 0;
    }

    static int lua_zstd_create_c_context(lua_State *L) {
        lua_settop(L, 1);
        int level = luaL_optinteger(L, 1, ZSTD_CLEVEL_DEFAULT);

        lua_settop(L, 0);

        ZSTD_CCtx * c_context = ZSTD_createCCtx();
        if (!c_context) {
            return luaL_error(L, "zstd error: can not create a CCtx");
        }

        size_t ret = ZSTD_CCtx_setParameter(c_context, ZSTD_c_compressionLevel, level);
        if (ZSTD_isError(ret)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(ret));
        }

        ZSTD_CCtx ** p_cctx = (ZSTD_CCtx **)lua_newuserdata(L, sizeof(void *));
        if (luaL_newmetatable(L, "zstd_c_context")) {
            *p_cctx = c_context;
            luaL_newlib(L, lua_zstd_c_context_functions);
            lua_setfield(L, -2, "__index");

            lua_pushcfunction(L, lua_zstd_c_context_tostring);
            lua_setfield(L, -2, "__tostring");

            lua_pushcfunction(L, lua_zstd_c_context_gc);
            lua_setfield(L, -2, "__gc");

            lua_setmetatable(L, -2);
            return 1;
        } else {
            ZSTD_freeCCtx(c_context);
            return luaL_error(L, "zstd error: can not create a zstd_c_context metatable");
        }
    }


    /**
     * ZSTD D Context
     * 
     */


    static int lua_zstd_d_context_decompress(lua_State *L) {
        lua_settop(L, 2);
        ZSTD_DCtx * ctx = check_d_context(L, 1);
        size_t data_len;
        const char * data = luaL_checklstring(L, 2, &data_len);

        lua_settop(L, 0);
        ZSTD_inBuffer input {data, data_len, 0};

        size_t buffer_out_size = ZSTD_DStreamOutSize();
        shared_ptr<char[]> buffer_out(new char[buffer_out_size]);
        vector<char> result;

        ZSTD_outBuffer output {buffer_out.get(), buffer_out_size, 0};

        size_t ret;
        while (input.pos < input.size || output.pos == output.size)
        {
            output.pos = 0;
            ret = ZSTD_decompressStream(ctx, &output, &input);

            if (ZSTD_isError(ret)) {
                return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(ret));
            }

            if (output.pos > 0) copy(buffer_out.get(), buffer_out.get() + output.pos, back_inserter(result));
        }
        
        lua_pushlstring(L, result.data(), result.size());
        lua_pushinteger(L, ret);
        return 2;
    }

    static int lua_zstd_d_context_reset(lua_State *L) {
        lua_settop(L, 2);
        ZSTD_DCtx * ctx = check_d_context(L, 1);
        ZSTD_ResetDirective flag = (ZSTD_ResetDirective)luaL_checkinteger(L, 2);

        lua_settop(L, 0);
        if (!ctx) return luaL_error(L, "ctx has already release");

        size_t ret = ZSTD_DCtx_reset(ctx, flag);
        if (ZSTD_isError(ret)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(ret));
        }

        lua_pushinteger(L, ret);
        return 1;
    }

    static int lua_zstd_d_context_load_dict(lua_State *L) {
        lua_settop(L, 2);
        ZSTD_DCtx * ctx = check_d_context(L, 1);
        size_t dict_len;
        const char * dict = luaL_checklstring(L, 2, &dict_len);

        lua_settop(L, 0);
        if (!ctx) return luaL_error(L, "ctx has already release");

        size_t ret = ZSTD_DCtx_loadDictionary(ctx, dict, dict_len);

        if (ZSTD_isError(ret)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(ret));
        }

        lua_pushinteger(L, ret);
        return 1;
    }

    static int lua_zstd_d_context_clear_dict(lua_State *L) {
        lua_settop(L, 1);
        ZSTD_DCtx * ctx = check_d_context(L, 1);

        lua_settop(L, 0);
        if (!ctx) return luaL_error(L, "ctx has already release");

        size_t ret = ZSTD_DCtx_refDDict(ctx, nullptr);

        if (ZSTD_isError(ret)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(ret));
        }

        lua_pushinteger(L, ret);
        return 1;
    }

    static int lua_zstd_d_context_ref_dict(lua_State *L) {
        lua_settop(L, 2);
        ZSTD_DCtx * ctx = check_d_context(L, 1);
        ZSTD_DDict * ddict = check_d_dict(L, 2);

        lua_settop(L, 0);
        if (!ctx) return luaL_error(L, "ctx has already release");
        if (!ddict) return luaL_error(L, "ddict has already release");

        size_t ret = ZSTD_DCtx_refDDict(ctx, ddict);
        
        if (ZSTD_isError(ret)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(ret));
        }

        lua_pushinteger(L, ret);
        return 1;
    }

    static const luaL_Reg lua_zstd_d_context_functions[] = {
        { "decompress", lua_zstd_d_context_decompress },
        { "reset", lua_zstd_d_context_reset },
        { "load_dict", lua_zstd_d_context_load_dict },
        { "clear_dict", lua_zstd_d_context_clear_dict },
        { "ref_dict", lua_zstd_d_context_ref_dict },
        { nullptr, nullptr }
    };

    static int lua_zstd_d_context_tostring(lua_State *L)
    {
        lua_settop(L, 1);
        ZSTD_DCtx * d_context = check_d_context(L, 1);

        lua_settop(L, 0);

        lua_pushfstring(L, "zstd_d_context: %p", d_context);
        return 1;
    }

    static int lua_zstd_d_context_gc(lua_State *L)
    {
        lua_settop(L, 1);
        ZSTD_DCtx ** p_d_context = (ZSTD_DCtx **)luaL_checkudata(L, 1, "zstd_d_context");
        ZSTD_DCtx * d_context = *p_d_context;

        lua_settop(L, 0);

        ZSTD_freeDCtx(d_context);
        *p_d_context = nullptr;

        return 0;
    }

    static int lua_zstd_create_d_context(lua_State *L) {
        lua_settop(L, 0);

        ZSTD_DCtx * d_context = ZSTD_createDCtx();
        if (!d_context) {
            return luaL_error(L, "zstd error: can not create a DCtx");
        }

        ZSTD_DCtx ** p_dctx = (ZSTD_DCtx **)lua_newuserdata(L, sizeof(void *));
        if (luaL_newmetatable(L, "zstd_d_context")) {
            *p_dctx = d_context;
            luaL_newlib(L, lua_zstd_d_context_functions);
            lua_setfield(L, -2, "__index");

            lua_pushcfunction(L, lua_zstd_d_context_tostring);
            lua_setfield(L, -2, "__tostring");

            lua_pushcfunction(L, lua_zstd_d_context_gc);
            lua_setfield(L, -2, "__gc");

            lua_setmetatable(L, -2);
            return 1;
        } else {
            ZSTD_freeDCtx(d_context);
            return luaL_error(L, "zstd error: can not create a zstd_d_context metatable");
        }
    }


    /**
     * ZSTD Global
     * 
     */

    static int lua_zstd_version_number(lua_State *L) {
        lua_settop(L, 0);

        lua_pushinteger(L, ZSTD_versionNumber());
        return 1;
    }

    static int lua_zstd_version(lua_State *L) {
        lua_settop(L, 0);

        lua_pushstring(L, ZSTD_versionString());
        return 1;
    }

    static int lua_zstd_compress(lua_State *L) {
        lua_settop(L, 2);
        size_t data_len;
        const char * data = luaL_checklstring(L, 1, &data_len);
        int level = luaL_optinteger(L, 2, ZSTD_CLEVEL_DEFAULT);

        lua_settop(L, 0);

        if (level < ZSTD_minCLevel() or level > ZSTD_maxCLevel()) return luaL_error(L, "zstd clevel not available, min level %d, max level %d", ZSTD_minCLevel(), ZSTD_maxCLevel());

        size_t dst_size = ZSTD_compressBound(data_len);
        if (ZSTD_isError(dst_size)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(dst_size));
        }

        char *buffer = new char[dst_size];
        size_t ret = ZSTD_compress(buffer, dst_size, data, data_len, level);
        if (ZSTD_isError(dst_size)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(dst_size));
        }

        lua_pushlstring(L, buffer, ret);
        delete [] buffer;
        return 1;
    }

    static int lua_zstd_decompress(lua_State *L) {
        lua_settop(L, 1);
        size_t data_len;
        const char * data = luaL_checklstring(L, 1, &data_len);

        lua_settop(L, 0);

        auto dst_size = ZSTD_getFrameContentSize(data, data_len);
        if (dst_size == ZSTD_CONTENTSIZE_UNKNOWN) {
            return luaL_error(L, "zstd decompress size cannot be determined, please use streaming mode decompress it");
        } else if (dst_size == ZSTD_CONTENTSIZE_ERROR) {
            return luaL_error(L, "zstd decompress size determined error");
        }

        shared_ptr<char[]> dst(new char[dst_size]);
        size_t ret = ZSTD_decompress(dst.get(), dst_size, data, data_len);
        if (ZSTD_isError(ret)) {
            return luaL_error(L, "zstd error: %s", ZSTD_getErrorName(dst_size));
        }

        lua_pushlstring(L, dst.get(), ret);
        return 1;
    }
    
    static const struct luaL_Reg zstd_funcs[] =
    {
        {"version_number", lua_zstd_version_number},
        {"version", lua_zstd_version},
        {"compress", lua_zstd_compress},
        {"decompress", lua_zstd_decompress},
        {"create_c_context", lua_zstd_create_c_context},
        {"create_d_context", lua_zstd_create_d_context},
        {"create_c_dict", lua_zstd_create_c_dict},
        {"create_d_dict", lua_zstd_create_d_dict},
        {nullptr, nullptr}
    };
    
    LUALIB_API int luaopen_zstd(lua_State * const L)
    {
      luaL_newlib(L, zstd_funcs);

      lua_pushinteger(L, ZSTD_e_continue);
      lua_setfield(L, -2, "E_CONTINUE");

      lua_pushinteger(L, ZSTD_e_end);
      lua_setfield(L, -2, "E_END");

      lua_pushinteger(L, ZSTD_e_flush);
      lua_setfield(L, -2, "E_FLUSH");

      lua_pushinteger(L, ZSTD_CONTENTSIZE_UNKNOWN);
      lua_setfield(L, -2, "CONTENTSIZE_UNKNOWN");

      lua_pushinteger(L, ZSTD_reset_parameters);
      lua_setfield(L, -2, "RESET_PARAMETERS");

      lua_pushinteger(L, ZSTD_reset_session_and_parameters);
      lua_setfield(L, -2, "RESET_SESSION_AND_PARAMETERS");

      lua_pushinteger(L, ZSTD_reset_session_only);
      lua_setfield(L, -2, "RESET_SESSION_ONLY");

      return 1;
    }
}