#include <stdint.h>

extern int64_t output_dbg(unsigned char* buf, int32_t len);
extern int64_t set_state    ( unsigned char* key_ptr, unsigned char* data_ptr_in, uint32_t in_len );
extern int64_t get_state    ( unsigned char* key_ptr, unsigned char* data_ptr_out, uint32_t out_len );
int64_t hook(int64_t reserved ) {
//    return output_dbg("hello world", 11);

    char* key =  "this right here is a 32 byte key";
    char* data = "some test data blah blah blah";
    
    char buffer[129];
    buffer[0] = '\0';
    get_state(key, buffer, 128);
    output_dbg(buffer, 128);
    set_state(key, data, 29);

    return 0;

}


int64_t* info_1(int32_t aaaa, int32_t bbbb) {

    return aaaa + bbbb;
}