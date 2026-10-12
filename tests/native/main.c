#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "ota.h"
#include "protocol.h"
typedef struct { uint8_t bytes[OTA_FLASH_SIZE]; int events,cut,fail,illegal; } sim_t;
static sim_t sim,baseline;
static jmp_buf power;
static int checks,failures;
#define CHECK(x) do { ++checks; if(!(x)) { printf("FAIL %s:%d: %s\n",__func__,__LINE__,#x); ++failures;return; } } while(0)
static uint32_t le32(const uint8_t*p) { return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
static void put32(uint8_t*p,uint32_t v) { p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24); }
static uint32_t reference_crc(const uint8_t*p,uint32_t n) { uint32_t c=0xFFFFFFFFUL,j;while(n--) { c^=*p++;for(j=0;j<8;j++) c=(c>>1)^((c&1)?0xEDB88320UL:0); }return ~c; }
static int bounds(uint32_t a,uint32_t n) { return a>=OTA_FLASH_BASE && a<=OTA_FLASH_BASE+OTA_FLASH_SIZE && n<=OTA_FLASH_BASE+OTA_FLASH_SIZE-a; }
static int event(sim_t*s) { ++s->events;if(s->cut==s->events) longjmp(power,1);return s->fail!=s->events; }
static int rd(void*u,uint32_t a,uint8_t*p,uint32_t n) { sim_t*s=u;if(!bounds(a,n))return 0;memcpy(p,s->bytes+a-OTA_FLASH_BASE,n);return 1; }
static int erase(void*u,uint32_t a) { sim_t*s=u;uint32_t j;if(a<OTA_APP_BASE || !bounds(a,OTA_PAGE_SIZE) || a%OTA_PAGE_SIZE) { s->illegal=1;return 0; }for(j=0;j<OTA_PAGE_SIZE;j+=2) { if(!event(s))return 0;s->bytes[a-OTA_FLASH_BASE+j]=255;s->bytes[a-OTA_FLASH_BASE+j+1]=255; }return 1; }
static int wr(void*u,uint32_t a,const uint8_t*p,uint32_t n) { sim_t*s=u;uint32_t j;uint8_t *d;if(a<OTA_APP_BASE || a%2 || !bounds(a,(n+1)&~1U)) { s->illegal=1;return 0; }d=s->bytes+a-OTA_FLASH_BASE;for(j=0;j<n;j+=2) { if(d[j]!=255 || d[j+1]!=255)return 0;if(!event(s))return 0;d[j]=p[j];d[j+1]=(j+1<n)?p[j+1]:255; }return 1; }
static ota_flash_t flash(void) { ota_flash_t f;f.user=&sim;f.read=rd;f.erase=erase;f.write=wr;return f; }
static void image(uint8_t*p,uint32_t n,uint32_t salt) { uint32_t j;for(j=0;j<n;j++)p[j]=(uint8_t)(j*37+salt);put32(p,OTA_RAM_END);put32(p+4,OTA_APP_BASE+9); }
static ota_image_t desc(const uint8_t*p,uint32_t n,uint32_t v) { ota_image_t i;i.length=n;i.crc=reference_crc(p,n);i.version=v;return i; }
static void factory(uint32_t generation) { uint8_t rec[64],old[257];ota_image_t i;memset(&sim,0,sizeof(sim));memset(sim.bytes,255,sizeof(sim.bytes));image(old,sizeof(old),13);i=desc(old,sizeof(old),1);memcpy(sim.bytes+OTA_APP_BASE-OTA_FLASH_BASE,old,sizeof(old));memset(rec,0,sizeof(rec));put32(rec,0x4F54414D);put32(rec+4,generation);put32(rec+8,OTA_VALID);put32(rec+16,i.length);put32(rec+20,i.crc);put32(rec+24,i.version);put32(rec+56,reference_crc(rec,56));rec[60]=0x5A;rec[61]=0xA5;rec[62]=255;rec[63]=255;memcpy(sim.bytes+OTA_META0-OTA_FLASH_BASE,rec,64); }
static int start(ota_t*c) { ota_flash_t f=flash();return ota_init(c,&f)==OTA_OK; }
static ota_result_t receive(ota_t*c,const uint8_t*p,uint32_t n,uint32_t version) { ota_image_t i=desc(p,n,version);ota_result_t r;uint32_t o=0;uint16_t q=1;r=ota_begin(c,77,OTA_MCU_ID,OTA_APP_BASE,&i);if(r)return r;while(o<n) { uint16_t k=(uint16_t)((n-o>128)?128:n-o);r=ota_data(c,77,q++,o,p+o,k);if(r)return r;o+=k; }return ota_finish(c,77); }
/* Catch incorrect polynomial, initial/final XOR or empty-input handling. */
static void test_crc(void) { CHECK(ota_crc32((const uint8_t*)"123456789",9)==0xCBF43926UL);CHECK(ota_crc32(NULL,0)==0); }
static void test_empty_factory_request(void) { ota_t c;ota_flash_t f=flash();memset(&sim,0,sizeof(sim));memset(sim.bytes,255,sizeof(sim.bytes));CHECK(ota_init(&c,&f)==OTA_OK);CHECK(c.state==OTA_EMPTY);CHECK(ota_validate_app(&c)!=OTA_OK);factory(1);CHECK(start(&c));CHECK(c.state==OTA_VALID);CHECK(ota_validate_app(&c)==OTA_OK);CHECK(ota_request_update(&c)==OTA_OK);CHECK(start(&c));CHECK(c.request==1);CHECK(c.app.version==1); }
static void test_normal_odd_install(void) { ota_t c;uint8_t p[389],old[257];factory(1);memcpy(old,sim.bytes+0x4000,sizeof(old));image(p,sizeof(p),27);CHECK(start(&c));CHECK(receive(&c,p,sizeof(p),2)==OTA_OK);CHECK(c.state==OTA_INSTALLING);CHECK(!memcmp(old,sim.bytes+0x4000,sizeof(old)));CHECK(ota_begin(&c,78,OTA_MCU_ID,OTA_APP_BASE,&c.candidate)==OTA_ERR_STATE);CHECK(ota_install(&c)==OTA_OK);CHECK(c.state==OTA_VALID && !c.request && c.app.version==2);CHECK(!memcmp(p,sim.bytes+0x4000,sizeof(p)));CHECK(sim.bytes[0x4000+389]==255);CHECK(sim.bytes[0x9BFF]==255);CHECK(start(&c));CHECK(ota_validate_app(&c)==OTA_OK);CHECK(!sim.illegal); }
static void test_rejection_duplicate_cancel(void) { ota_t c;uint8_t p[389],alter[128],old[257];ota_image_t i;int events;factory(1);image(p,sizeof(p),8);i=desc(p,sizeof(p),2);memcpy(old,sim.bytes+0x4000,sizeof(old));CHECK(start(&c));CHECK(ota_begin(&c,77,0,OTA_APP_BASE,&i)==OTA_ERR_TARGET);CHECK(ota_begin(&c,77,OTA_MCU_ID,OTA_APP_BASE+1024,&i)==OTA_ERR_TARGET);i.length=0;CHECK(ota_begin(&c,77,OTA_MCU_ID,OTA_APP_BASE,&i)==OTA_ERR_LENGTH);i.length=OTA_APP_SIZE+1;CHECK(ota_begin(&c,77,OTA_MCU_ID,OTA_APP_BASE,&i)==OTA_ERR_LENGTH);i=desc(p,sizeof(p),2);CHECK(ota_begin(&c,77,OTA_MCU_ID,OTA_APP_BASE,&i)==OTA_OK);CHECK(ota_data(&c,78,1,0,p,128)==OTA_ERR_SESSION);CHECK(ota_data(&c,77,2,0,p,128)==OTA_ERR_SEQUENCE);CHECK(ota_data(&c,77,1,1,p,128)==OTA_ERR_OFFSET);CHECK(ota_data(&c,77,1,0,p,0)==OTA_ERR_LENGTH);CHECK(ota_data(&c,77,1,0,p,129)==OTA_ERR_LENGTH);CHECK(ota_data(&c,77,1,0,p,127)==OTA_ERR_LENGTH);CHECK(ota_data(&c,77,1,0,p,128)==OTA_OK);events=sim.events;CHECK(ota_begin(&c,77,OTA_MCU_ID,OTA_APP_BASE,&i)==OTA_OK);CHECK(sim.events==events && c.received==128);CHECK(ota_data(&c,77,1,0,p,128)==OTA_OK);CHECK(sim.events==events);memcpy(alter,p,128);alter[20]^=1;CHECK(ota_data(&c,77,1,0,alter,128)==OTA_ERR_CONTENT);CHECK(ota_finish(&c,77)==OTA_ERR_LENGTH);CHECK(ota_cancel(&c,78)==OTA_ERR_SESSION);CHECK(ota_cancel(&c,77)==OTA_OK);CHECK(c.state==OTA_VALID && c.request==0);CHECK(!memcmp(old,sim.bytes+0x4000,sizeof(old))); }
static void test_receive_reboot(void) { ota_t c;uint8_t p[389],old[257];ota_image_t i;factory(1);image(p,sizeof(p),9);i=desc(p,sizeof(p),2);memcpy(old,sim.bytes+0x4000,sizeof(old));CHECK(start(&c));CHECK(ota_begin(&c,77,OTA_MCU_ID,OTA_APP_BASE,&i)==OTA_OK);CHECK(ota_data(&c,77,1,0,p,128)==OTA_OK);CHECK(start(&c));CHECK(c.state==OTA_RECEIVING && c.session==0 && c.received==0);CHECK(ota_data(&c,77,2,128,p+128,128)==OTA_ERR_SESSION);CHECK(!memcmp(old,sim.bytes+0x4000,sizeof(old)));CHECK(receive(&c,p,sizeof(p),2)==OTA_OK);CHECK(ota_install(&c)==OTA_OK); }
static void test_vector_crc_failures(void) { ota_t c;uint8_t p[389];uint32_t vectors[][2]={{OTA_RAM_BASE,OTA_APP_BASE+9},{OTA_RAM_END+8,OTA_APP_BASE+9},{OTA_RAM_END-1,OTA_APP_BASE+9},{OTA_RAM_END,OTA_APP_BASE+8},{OTA_RAM_END,OTA_STAGE_BASE+9},{OTA_RAM_END,OTA_APP_BASE+391}};uint32_t j;for(j=0;j<sizeof(vectors)/sizeof(vectors[0]);j++) { factory(1);image(p,sizeof(p),9);put32(p,vectors[j][0]);put32(p+4,vectors[j][1]);CHECK(start(&c));CHECK(receive(&c,p,sizeof(p),2)==OTA_ERR_VECTOR);CHECK(c.state==OTA_RECEIVING);CHECK(ota_validate_app(&c)==OTA_OK); }factory(1);image(p,sizeof(p),9);CHECK(start(&c));{ota_image_t i=desc(p,sizeof(p),2);i.crc^=1;CHECK(ota_begin(&c,77,OTA_MCU_ID,OTA_APP_BASE,&i)==OTA_OK);for(j=0;j<sizeof(p);j+=128) CHECK(ota_data(&c,77,(uint16_t)(1+j/128),j,p+j,(uint16_t)((sizeof(p)-j>128)?128:sizeof(p)-j))==OTA_OK);CHECK(ota_finish(&c,77)==OTA_ERR_CRC);}factory(1);CHECK(start(&c));CHECK(receive(&c,p,sizeof(p),2)==OTA_OK);sim.bytes[OTA_STAGE_BASE-OTA_FLASH_BASE+sizeof(p)-1]^=1;CHECK(ota_install(&c)==OTA_ERR_CRC);CHECK(c.state==OTA_INSTALLING);CHECK(ota_validate_app(&c)==OTA_OK); }
static void test_metadata_failures(void) { ota_t c;int k;factory(1);baseline=sim;for(k=1;k<=31;k++) { sim=baseline;CHECK(start(&c));sim.fail=k;CHECK(ota_request_update(&c)==OTA_ERR_FLASH);CHECK(c.state==OTA_VALID && c.request==0);sim.fail=0;CHECK(start(&c));CHECK(c.request==0);CHECK(ota_request_update(&c)==OTA_OK);CHECK(c.request==1); } }
static void test_power_copy(void) { ota_t c;uint8_t p[389];int total,k;factory(1);image(p,sizeof(p),29);CHECK(start(&c));CHECK(receive(&c,p,sizeof(p),2)==OTA_OK);sim.events=0;baseline=sim;CHECK(ota_install(&c)==OTA_OK);total=sim.events;for(k=1;k<=total+1;k++) { sim=baseline;sim.cut=k;if(setjmp(power)==0) { if(!start(&c)) { CHECK(0); }(void)ota_install(&c); }sim.cut=0;CHECK(start(&c));CHECK(c.state==OTA_INSTALLING || c.state==OTA_VALID);if(c.state==OTA_INSTALLING)CHECK(ota_install(&c)==OTA_OK);CHECK(ota_validate_app(&c)==OTA_OK);CHECK(c.app.version==2);CHECK(!memcmp(p,sim.bytes+0x4000,sizeof(p)));CHECK(!sim.illegal); }printf("copy power cuts: %d\n",total+1); }
static void test_power_journal(void) {
    ota_t c;int total,j;volatile int k,round;uint32_t previous;
    /* Retain the nested-loop position after an injected longjmp. The context
     * itself is cold-initialized before any of its fields are read again. */
    for(round=0;round<2;round++) {
        factory(0xFFFFFFF0UL);CHECK(start(&c));for(j=0;j<31+round*16;j++)CHECK(ota_request_update(&c)==OTA_OK);
        CHECK(c.generation==(uint32_t)(15+round*16));previous=c.generation;sim.events=0;baseline=sim;
        CHECK(ota_request_update(&c)==OTA_OK);total=sim.events;
        for(k=1;k<=total+1;k++) {
            sim=baseline;sim.cut=k;
            if(setjmp(power)==0) { if(!start(&c))CHECK(0);(void)ota_request_update(&c); }
            sim.cut=0;CHECK(start(&c));CHECK(c.state==OTA_VALID && c.request==1);
            CHECK(c.generation==previous || c.generation==previous+1);CHECK(ota_validate_app(&c)==OTA_OK);
            CHECK(ota_request_update(&c)==OTA_OK);CHECK(!sim.illegal);
        }
        printf("journal rollover direction %d power cuts: %d\n",round,total+1);
    }
}
static void test_protocol(void) { ota_parser_t parser;ota_frame_t f,out;uint8_t encoded[150],bad[150];uint16_t n,j;int got=0;memset(&f,0,sizeof(f));f.type=OTA_BEGIN;f.session=0x12345678;f.length=20;put32(f.payload,OTA_MCU_ID);put32(f.payload+4,OTA_APP_BASE);put32(f.payload+8,389);put32(f.payload+12,0x87654321);put32(f.payload+16,2);n=ota_frame_encode(&f,encoded);CHECK(n==38);CHECK(!memcmp(encoded,"GBOT\1\2\x78\x56\x34\x12\0\0\x14\0",14));CHECK(le32(encoded+n-4)==reference_crc(encoded,n-4));ota_parser_init(&parser);CHECK(!ota_parser_feed(&parser,'x',&out));CHECK(!parser.active);CHECK(!ota_parser_feed(&parser,'G',&out));CHECK(parser.active);CHECK(!ota_parser_feed(&parser,'G',&out));for(j=0;j<n;j++)got+=ota_parser_feed(&parser,encoded[j],&out);CHECK(got==1);CHECK(out.type==f.type && out.session==f.session && out.length==20);CHECK(!memcmp(out.payload,f.payload,20));CHECK(!parser.active);memcpy(bad,encoded,n);bad[n-1]^=1;for(j=0;j<n;j++)CHECK(!ota_parser_feed(&parser,bad[j],&out));got=0;for(j=0;j<n;j++)got+=ota_parser_feed(&parser,encoded[j],&out);CHECK(got==1);memcpy(bad,encoded,n);bad[12]=133;bad[13]=0;for(j=0;j<14;j++)CHECK(!ota_parser_feed(&parser,bad[j],&out));got=0;for(j=0;j<n;j++)got+=ota_parser_feed(&parser,encoded[j],&out);CHECK(got==1);f.length=133;CHECK(ota_frame_encode(&f,encoded)==0); }
static void test_dispatch(void) { ota_t c;ota_frame_t f,r;factory(1);CHECK(start(&c));memset(&f,0,sizeof(f));f.type=OTA_QUERY;f.session=56;f.sequence=9;CHECK(ota_handle_frame(&c,&f,&r,0)==1);CHECK(r.type==OTA_ACK && r.length==28 && r.session==56 && r.sequence==9);CHECK(le32(r.payload+16)==1 && le32(r.payload+20)==257);f.type=OTA_BEGIN;f.length=20;CHECK(ota_handle_frame(&c,&f,&r,0)==1);CHECK(r.type==OTA_NACK && le32(r.payload)==OTA_ERR_STATE); }
static int hex_digit(int ch) { if(ch>='0' && ch<='9')return ch-'0';if(ch>='a' && ch<='f')return ch-'a'+10;if(ch>='A' && ch<='F')return ch-'A'+10;return -1; }
static uint16_t process_hex_line(ota_t*c,ota_parser_t*p,const char*line,uint8_t*out) {
    int high=-1,value;uint16_t result=0;ota_frame_t frame,response;
    for(;*line;line++) {
        if(*line==' ' || *line=='\t' || *line=='\r' || *line=='\n')continue;
        value=hex_digit((unsigned char)*line);if(value<0) { ota_parser_init(p);return 0; }
        if(high<0)high=value;
        else { if(ota_parser_feed(p,(uint8_t)((high<<4)|value),&frame) && ota_handle_frame(c,&frame,&response,1))result=ota_frame_encode(&response,out);high=-1; }
    }
    if(high>=0) { ota_parser_init(p);return 0; }return result;
}
static void test_serve_line(void) {
    ota_t c;ota_parser_t parser;uint8_t response[150];
    factory(1);CHECK(start(&c));ota_parser_init(&parser);
    CHECK(process_hex_line(&c,&parser,"47424f54010100000000000000004cf9375a",response)==46);
    CHECK(response[5]==OTA_ACK && le32(response+30)==1 && le32(response+34)==257);
}
static void test_application_query_flag(void) {
    ota_t c;ota_frame_t f,r;factory(1);CHECK(start(&c));memset(&f,0,sizeof(f));f.type=OTA_QUERY;
    CHECK(ota_handle_frame(&c,&f,&r,0)==1);CHECK(le32(r.payload+8)==0x80000001UL);
    CHECK(ota_handle_frame(&c,&f,&r,1)==1);CHECK(le32(r.payload+8)==1);
}
/* Catch copying before INSTALLING is committed and loss of the old app on receive. */
static void test_power_receive(void) {
    ota_t c;uint8_t p[389],old[257];int total,k;
    factory(1);image(p,sizeof(p),33);memcpy(old,sim.bytes+0x4000,sizeof(old));CHECK(start(&c));
    sim.events=0;baseline=sim;CHECK(receive(&c,p,sizeof(p),2)==OTA_OK);total=sim.events;
    for(k=1;k<=total+1;k++) {
        sim=baseline;sim.cut=k;
        if(setjmp(power)==0) { if(!start(&c))CHECK(0);(void)receive(&c,p,sizeof(p),2); }
        sim.cut=0;CHECK(start(&c));CHECK(!memcmp(old,sim.bytes+0x4000,sizeof(old)));
        CHECK(c.state==OTA_VALID || c.state==OTA_RECEIVING || c.state==OTA_INSTALLING);
        CHECK(c.session==0 && c.received==0);
        if(c.state!=OTA_INSTALLING)CHECK(receive(&c,p,sizeof(p),2)==OTA_OK);
        CHECK(ota_install(&c)==OTA_OK);CHECK(ota_validate_app(&c)==OTA_OK);CHECK(c.app.version==2);CHECK(!sim.illegal);
    }
    printf("receive power cuts: %d\n",total+1);
}
static void test_dispatch_install(void) {
    ota_t c;ota_frame_t f,r;uint8_t p[389];ota_image_t i;uint32_t o;uint16_t q=1;
    factory(1);image(p,sizeof(p),4);i=desc(p,sizeof(p),2);CHECK(start(&c));memset(&f,0,sizeof(f));
    f.type=OTA_BEGIN;f.session=77;f.length=20;put32(f.payload,OTA_MCU_ID);put32(f.payload+4,OTA_APP_BASE);
    put32(f.payload+8,i.length);put32(f.payload+12,i.crc);put32(f.payload+16,i.version);
    f.sequence=1;CHECK(ota_handle_frame(&c,&f,&r,1)==1);CHECK(r.type==OTA_NACK && le32(r.payload)==OTA_ERR_SEQUENCE);
    f.sequence=0;CHECK(ota_handle_frame(&c,&f,&r,1)==1);CHECK(r.type==OTA_ACK);
    for(o=0;o<sizeof(p);o+=128) {
        f.type=OTA_DATA;f.sequence=q++;f.length=(uint16_t)(4+((sizeof(p)-o>128)?128:sizeof(p)-o));
        put32(f.payload,o);memcpy(f.payload+4,p+o,f.length-4);CHECK(ota_handle_frame(&c,&f,&r,1)==1);CHECK(r.type==OTA_ACK);
    }
    f.type=OTA_COMMIT;f.length=0;f.sequence=q+1;CHECK(ota_handle_frame(&c,&f,&r,1)==1);CHECK(r.type==OTA_NACK && le32(r.payload)==OTA_ERR_SEQUENCE);
    f.sequence=q;CHECK(ota_handle_frame(&c,&f,&r,1)==1);CHECK(r.type==OTA_DONE && le32(r.payload)==0);
    CHECK(c.state==OTA_VALID && c.app.version==2);CHECK(ota_handle_frame(&c,&f,&r,1)==1);CHECK(r.type==OTA_DONE);
    CHECK(!memcmp(p,sim.bytes+0x4000,sizeof(p)));
}
static void test_empty_cancel_and_limit(void) {
    ota_t c;ota_flash_t f=flash();uint8_t p[OTA_APP_SIZE];ota_image_t i;
    memset(&sim,0,sizeof(sim));memset(sim.bytes,255,sizeof(sim.bytes));CHECK(ota_init(&c,&f)==OTA_OK);
    image(p,sizeof(p),3);i=desc(p,sizeof(p),2);CHECK(ota_begin(&c,77,OTA_MCU_ID,OTA_APP_BASE,&i)==OTA_OK);
    CHECK(ota_cancel(&c,77)==OTA_OK);CHECK(c.state==OTA_EMPTY && c.request==0);
    CHECK(receive(&c,p,sizeof(p),2)==OTA_OK);CHECK(ota_install(&c)==OTA_OK);CHECK(ota_validate_app(&c)==OTA_OK);
    CHECK(!memcmp(p,sim.bytes+0x4000,sizeof(p)));CHECK(!memcmp(p,sim.bytes+0x9C00,sizeof(p)));CHECK(!sim.illegal);
}
static void test_partial_data_failure(void) {
    ota_t c;uint8_t p[389],old[257];ota_image_t i;
    factory(1);image(p,sizeof(p),9);i=desc(p,sizeof(p),2);memcpy(old,sim.bytes+0x4000,sizeof(old));CHECK(start(&c));
    CHECK(ota_begin(&c,77,OTA_MCU_ID,OTA_APP_BASE,&i)==OTA_OK);sim.fail=sim.events+12;
    CHECK(ota_data(&c,77,1,0,p,128)==OTA_ERR_FLASH);CHECK(!memcmp(old,sim.bytes+0x4000,sizeof(old)));
    CHECK(ota_data(&c,77,1,0,p,128)==OTA_ERR_SESSION);sim.fail=0;CHECK(receive(&c,p,sizeof(p),2)==OTA_OK);CHECK(ota_install(&c)==OTA_OK);
}
static void test_journal_sanity(void) {
    ota_t c;uint8_t *record;uint32_t invalid[][2]={{8,4},{12,2},{16,OTA_APP_SIZE+1},{28,8},{40,1}};uint32_t j;
    for(j=0;j<sizeof(invalid)/sizeof(invalid[0]);j++) {
        factory(1);record=sim.bytes+OTA_META0-OTA_FLASH_BASE+64;memcpy(record,record-64,64);put32(record+4,99);
        put32(record+invalid[j][0],invalid[j][1]);put32(record+56,reference_crc(record,56));
        CHECK(start(&c));CHECK(c.generation==1 && c.state==OTA_VALID);CHECK(ota_request_update(&c)==OTA_OK);CHECK(c.journal_slot==2);
    }
    factory(1);record=sim.bytes+OTA_META0-OTA_FLASH_BASE+64;memcpy(record,record-64,64);put32(record+4,99);
    put32(record+56,reference_crc(record,56));record[60]=255;CHECK(start(&c));CHECK(c.generation==1);
}
static int tests(void) { test_crc();test_empty_factory_request();test_normal_odd_install();test_rejection_duplicate_cancel();test_receive_reboot();test_vector_crc_failures();test_metadata_failures();test_protocol();test_dispatch();test_serve_line();test_application_query_flag();test_dispatch_install();test_empty_cancel_and_limit();test_partial_data_failure();test_journal_sanity();test_power_receive();test_power_copy();test_power_journal();printf("%d checks, %d failed tests\n",checks,failures);return failures?1:0; }
static int serve(void) {
    ota_t c;ota_parser_t parser;char line[1024];uint8_t response[150];uint16_t n,j;
    factory(1);if(!start(&c))return 2;ota_parser_init(&parser);
    while(fgets(line,sizeof(line),stdin)) { n=process_hex_line(&c,&parser,line,response);for(j=0;j<n;j++)printf("%02x",response[j]);putchar('\n');fflush(stdout); }
    return 0;
}
int main(int argc,char**argv) { if(argc>1 && !strcmp(argv[1],"serve"))return serve();if(argc>1 && strcmp(argv[1],"test")) { fprintf(stderr,"usage: ota_native [test|serve]\n");return 2; }return tests(); }
