#include "protocol.h"
#include <string.h>
static uint16_t get16(const uint8_t*p) { return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8)); }
static uint32_t get32(const uint8_t*p) { return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
static void put16(uint8_t*p,uint16_t v) { p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8); }
static void put32(uint8_t*p,uint32_t v) { p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24); }
void ota_parser_init(ota_parser_t*p) { memset(p,0,sizeof(*p)); }
static void discard(ota_parser_t*p,uint16_t n) { p->used=(uint16_t)(p->used-n);memmove(p->buffer,p->buffer+n,p->used); }
/* Retain suffixes that can start GBOT, including magic embedded in a bad frame. */
int ota_parser_feed(ota_parser_t*p,uint8_t byte,ota_frame_t*f) {
    static const uint8_t magic[4]={'G','B','O','T'};uint16_t j,length,total;
    if(!p || !f)return 0;
    if(p->used>=150)discard(p,1);
    p->buffer[p->used++]=byte;
    for(;;) {
        for(j=0;j<p->used && j<4;j++)if(p->buffer[j]!=magic[j])break;
        if(j<p->used && j<4) { discard(p,1);continue; }
        if(p->used<5)break;
        if(p->buffer[4]!=1) { discard(p,1);continue; }
        if(p->used<14)break;
        length=get16(p->buffer+12);
        if(length>132) { discard(p,1);continue; }
        total=(uint16_t)(18+length);if(p->used<total)break;
        if(get32(p->buffer+total-4)!=ota_crc32(p->buffer,total-4)) { discard(p,1);continue; }
        f->type=p->buffer[5];f->session=get32(p->buffer+6);f->sequence=get16(p->buffer+10);f->length=length;
        memcpy(f->payload,p->buffer+14,length);discard(p,total);p->active=p->used!=0;return 1;
    }
    p->active=p->used!=0;return 0;
}
uint16_t ota_frame_encode(const ota_frame_t*f,uint8_t*p) {
    uint16_t n;if(!f || !p || f->length>132)return 0;n=(uint16_t)(14+f->length);
    memcpy(p,"GBOT",4);p[4]=1;p[5]=f->type;put32(p+6,f->session);put16(p+10,f->sequence);put16(p+12,f->length);
    memcpy(p+14,f->payload,f->length);put32(p+n,ota_crc32(p,n));return (uint16_t)(n+4);
}
int ota_handle_frame(ota_t*c,const ota_frame_t*f,ota_frame_t*r,int allow_updates) {
    ota_result_t error=OTA_ERR_STATE;uint8_t type=OTA_ACK;uint32_t info_state;
    if(!c || !f || !r)return 0;
    if(f->type==OTA_QUERY)error=f->length?OTA_ERR_LENGTH:OTA_OK;
#ifndef OTA_APPLICATION
    else if(allow_updates) {
        switch(f->type) {
        case OTA_BEGIN:
            if(f->length!=20)error=OTA_ERR_LENGTH;
            else if(f->sequence)error=OTA_ERR_SEQUENCE;
            else { ota_image_t i;i.length=get32(f->payload+8);i.crc=get32(f->payload+12);i.version=get32(f->payload+16);error=ota_begin(c,f->session,get32(f->payload),get32(f->payload+4),&i); }
            break;
        case OTA_DATA:
            error=f->length<5 || f->length>132?OTA_ERR_LENGTH:ota_data(c,f->session,f->sequence,get32(f->payload),f->payload+4,(uint16_t)(f->length-4));break;
        case OTA_COMMIT:
            if(f->length)error=OTA_ERR_LENGTH;
            else if(!c->session || f->session!=c->session)error=OTA_ERR_SESSION;
            else if(f->sequence!=c->expected_seq)error=OTA_ERR_SEQUENCE;
            else if(c->state==OTA_VALID && c->received==c->app.length) { error=ota_validate_app(c);if(error==OTA_OK)type=OTA_DONE; }
            else { error=c->state==OTA_INSTALLING?OTA_OK:ota_finish(c,f->session);if(error==OTA_OK)error=ota_install(c);if(error==OTA_OK)type=OTA_DONE; }
            break;
        case OTA_CANCEL:
            if(f->length)error=OTA_ERR_LENGTH;
            else if(f->sequence!=c->expected_seq)error=OTA_ERR_SEQUENCE;
            else error=ota_cancel(c,f->session);
            break;
        default:break;
        }
    }
#else
    (void)allow_updates;
#endif
    r->type=error==OTA_OK?type:OTA_NACK;r->session=f->session;r->sequence=f->sequence;r->length=28;
    info_state=c->state;
#ifdef OTA_APPLICATION
    info_state|=OTA_INFO_APPLICATION;
#else
    if(!allow_updates)info_state|=OTA_INFO_APPLICATION;
#endif
    put32(r->payload,error);put32(r->payload+4,c->received);put32(r->payload+8,info_state);put32(r->payload+12,c->request);
    put32(r->payload+16,c->app.version);put32(r->payload+20,c->app.length);put32(r->payload+24,c->app.crc);
    return 1;
}
