#include "ota.h"
#include <string.h>
#define RECORD_MAGIC 0x4F54414DUL
static uint32_t get32(const uint8_t*p) { return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
static void put32(uint8_t*p,uint32_t v) { p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24); }
static uint32_t crc_update(uint32_t crc,const uint8_t*p,uint32_t n) { uint32_t j;while(n--) { crc^=*p++;for(j=0;j<8;j++)crc=(crc>>1)^((crc&1U)?0xEDB88320UL:0U); }return crc; }
uint32_t ota_crc32(const uint8_t*p,uint32_t n) { return ~crc_update(0xFFFFFFFFUL,p,n); }
static uint32_t slot_address(uint16_t slot) { return OTA_META0+(uint32_t)slot*64U; }
static int valid_length(uint32_t n) { return n>=8 && n<=OTA_APP_SIZE; }
static int zero_image(const ota_image_t*i) { return !i->length && !i->crc && !i->version; }
static int newer(uint32_t a,uint32_t b) { uint32_t d=a-b;return d!=0 && d<0x80000000UL; }
static int record_valid(const uint8_t*r) {
    uint32_t s=get32(r+8),req=get32(r+12),alen=get32(r+16),clen=get32(r+28),j;
    if(get32(r)!=RECORD_MAGIC || r[60]!=0x5A || r[61]!=0xA5 || r[62]!=255 || r[63]!=255 || get32(r+56)!=ota_crc32(r,56))return 0;
    if(s>OTA_INSTALLING || req>1)return 0;
    for(j=40;j<56;j++)if(r[j])return 0;
    if(alen && !valid_length(alen))return 0;
    if(!alen && (get32(r+20) || get32(r+24)))return 0;
    if(s==OTA_EMPTY && alen)return 0;
    if(s==OTA_VALID && !valid_length(alen))return 0;
    if(s==OTA_RECEIVING || s==OTA_INSTALLING) { if(!valid_length(clen))return 0; }
    else if(clen || get32(r+32) || get32(r+36))return 0;
    return 1;
}
static void record_load(ota_t*c,const uint8_t*r,int16_t slot) {
    c->generation=get32(r+4);c->state=get32(r+8);c->request=get32(r+12);
    c->app.length=get32(r+16);c->app.crc=get32(r+20);c->app.version=get32(r+24);
    c->candidate.length=get32(r+28);c->candidate.crc=get32(r+32);c->candidate.version=get32(r+36);c->journal_slot=slot;
}
ota_result_t ota_init(ota_t*c,const ota_flash_t*f) {
    ota_flash_t copy;uint8_t r[64];uint16_t j;
    if(!c || !f || !f->read || !f->write || !f->erase)return OTA_ERR_FLASH;
    copy=*f;memset(c,0,sizeof(*c));c->flash=copy;c->journal_slot=-1;c->expected_seq=1;
    for(j=0;j<32;j++) {
        if(!c->flash.read(c->flash.user,slot_address(j),r,64))return OTA_ERR_FLASH;
        if(record_valid(r) && (c->journal_slot<0 || newer(get32(r+4),c->generation)))record_load(c,r,(int16_t)j);
    }
    c->usable=1;return OTA_OK;
}
static ota_result_t metadata_error(ota_t*c) { ota_flash_t f=c->flash;(void)ota_init(c,&f);return OTA_ERR_FLASH; }
static int erased(const uint8_t*r) { uint32_t j;for(j=0;j<64;j++)if(r[j]!=255)return 0;return 1; }
/* The separately programmed commit marker makes a record authoritative. */
static ota_result_t persist(ota_t*c,const ota_t*next) {
    uint8_t r[64],verify[64];uint16_t page,j;int16_t slot=-1;uint32_t generation;
    if(!c->usable)return OTA_ERR_FLASH;
    page=c->journal_slot<0?0:(uint16_t)(c->journal_slot/16);
    for(j=page*16;j<(page+1)*16;j++) {
        if(!c->flash.read(c->flash.user,slot_address(j),verify,64))return metadata_error(c);
        if(erased(verify)) { slot=(int16_t)j;break; }
    }
    if(slot<0) {
        page=(uint16_t)(1-page);
        for(j=page*16;j<(page+1)*16;j++) {
            if(!c->flash.read(c->flash.user,slot_address(j),verify,64))return metadata_error(c);
            if(erased(verify)) { slot=(int16_t)j;break; }
        }
        if(slot<0) {
            if(!c->flash.erase(c->flash.user,slot_address(page*16)))return metadata_error(c);
            slot=(int16_t)(page*16);
        }
    }
    generation=c->journal_slot<0?1:c->generation+1;
    memset(r,0,sizeof(r));put32(r,RECORD_MAGIC);put32(r+4,generation);put32(r+8,next->state);put32(r+12,next->request);
    put32(r+16,next->app.length);put32(r+20,next->app.crc);put32(r+24,next->app.version);
    put32(r+28,next->candidate.length);put32(r+32,next->candidate.crc);put32(r+36,next->candidate.version);
    put32(r+56,ota_crc32(r,56));r[60]=0x5A;r[61]=0xA5;r[62]=255;r[63]=255;
    if(!c->flash.write(c->flash.user,slot_address((uint16_t)slot),r,60))return metadata_error(c);
    if(!c->flash.read(c->flash.user,slot_address((uint16_t)slot),verify,60) || memcmp(r,verify,60))return metadata_error(c);
    if(!c->flash.write(c->flash.user,slot_address((uint16_t)slot)+60,r+60,2))return metadata_error(c);
    if(!c->flash.read(c->flash.user,slot_address((uint16_t)slot),verify,64) || memcmp(r,verify,64))return metadata_error(c);
    *c=*next;c->generation=generation;c->journal_slot=slot;return OTA_OK;
}
ota_result_t ota_request_update(ota_t*c) { ota_t next;if(!c || !c->usable)return OTA_ERR_FLASH;next=*c;next.request=1;return persist(c,&next); }
static ota_result_t validate_image(ota_t*c,uint32_t base,const ota_image_t*i) {
    uint8_t buffer[128],vectors[8];uint32_t offset=0,crc=0xFFFFFFFFUL,msp,reset,n;
    if(!valid_length(i->length))return OTA_ERR_LENGTH;
    while(offset<i->length) {
        n=i->length-offset;if(n>128)n=128;
        if(!c->flash.read(c->flash.user,base+offset,buffer,n))return OTA_ERR_FLASH;
        crc=crc_update(crc,buffer,n);if(!offset)memcpy(vectors,buffer,8);offset+=n;
    }
    if(~crc!=i->crc)return OTA_ERR_CRC;
    msp=get32(vectors);reset=get32(vectors+4);
    if(msp<=OTA_RAM_BASE || msp>OTA_RAM_END || (msp&7) || !(reset&1) || (reset&~1UL)<OTA_APP_BASE || (reset&~1UL)>=OTA_APP_BASE+i->length)return OTA_ERR_VECTOR;
    return OTA_OK;
}
ota_result_t ota_validate_app(ota_t*c) { if(!c || !c->usable)return OTA_ERR_FLASH;return validate_image(c,OTA_APP_BASE,&c->app); }
ota_result_t ota_begin(ota_t*c,uint32_t session,uint32_t mcu,uint32_t base,const ota_image_t*i) {
    ota_t next;uint32_t offset;
    if(!c || !c->usable)return OTA_ERR_FLASH;
    if(c->state==OTA_INSTALLING)return OTA_ERR_STATE;
    if(mcu!=OTA_MCU_ID || base!=OTA_APP_BASE)return OTA_ERR_TARGET;
    if(!i || !valid_length(i->length))return OTA_ERR_LENGTH;
    if(!session)return OTA_ERR_SESSION;
    if(c->state==OTA_RECEIVING && c->session==session && c->candidate.length==i->length && c->candidate.crc==i->crc && c->candidate.version==i->version)return OTA_OK;
    for(offset=0;offset<OTA_APP_SIZE;offset+=OTA_PAGE_SIZE)if(!c->flash.erase(c->flash.user,OTA_STAGE_BASE+offset)) { c->session=0;c->received=0;return OTA_ERR_FLASH; }
    next=*c;next.state=OTA_RECEIVING;next.request=1;next.candidate=*i;next.received=0;next.session=session;next.expected_seq=1;next.last_length=0;
    return persist(c,&next);
}
ota_result_t ota_data(ota_t*c,uint32_t session,uint16_t seq,uint32_t offset,const uint8_t*p,uint16_t n) {
    if(!c || !c->usable)return OTA_ERR_FLASH;
    if(c->state!=OTA_RECEIVING)return OTA_ERR_STATE;
    if(!c->session || session!=c->session)return OTA_ERR_SESSION;
    if(!p || !n || n>128)return OTA_ERR_LENGTH;
    if(c->last_length && seq==c->last_seq) {
        if(offset!=c->last_offset)return OTA_ERR_OFFSET;
        if(n!=c->last_length || memcmp(p,c->last_data,n))return OTA_ERR_CONTENT;
        return OTA_OK;
    }
    if(seq!=c->expected_seq)return OTA_ERR_SEQUENCE;
    if(offset!=c->received)return OTA_ERR_OFFSET;
    if(offset>c->candidate.length || n>c->candidate.length-offset || (n!=128 && offset+n!=c->candidate.length))return OTA_ERR_LENGTH;
    if(!c->flash.write(c->flash.user,OTA_STAGE_BASE+offset,p,n)) { c->session=0;c->received=0;return OTA_ERR_FLASH; }
    c->last_seq=seq;c->last_offset=offset;c->last_length=n;memcpy(c->last_data,p,n);c->received+=n;c->expected_seq++;
    return OTA_OK;
}
ota_result_t ota_finish(ota_t*c,uint32_t session) {
    ota_t next;ota_result_t r;
    if(!c || !c->usable)return OTA_ERR_FLASH;
    if(c->state!=OTA_RECEIVING)return OTA_ERR_STATE;
    if(!c->session || c->session!=session)return OTA_ERR_SESSION;
    if(c->received!=c->candidate.length)return OTA_ERR_LENGTH;
    r=validate_image(c,OTA_STAGE_BASE,&c->candidate);if(r)return r;
    next=*c;next.state=OTA_INSTALLING;return persist(c,&next);
}
ota_result_t ota_install(ota_t*c) {
    ota_t next;uint8_t buffer[128];uint32_t offset,n;ota_result_t r;
    if(!c || !c->usable)return OTA_ERR_FLASH;
    if(c->state!=OTA_INSTALLING)return OTA_ERR_STATE;
    r=validate_image(c,OTA_STAGE_BASE,&c->candidate);if(r)return r;
    for(offset=0;offset<OTA_APP_SIZE;offset+=OTA_PAGE_SIZE)if(!c->flash.erase(c->flash.user,OTA_APP_BASE+offset))return OTA_ERR_FLASH;
    for(offset=0;offset<c->candidate.length;offset+=n) {
        n=c->candidate.length-offset;if(n>128)n=128;
        if(!c->flash.read(c->flash.user,OTA_STAGE_BASE+offset,buffer,n) || !c->flash.write(c->flash.user,OTA_APP_BASE+offset,buffer,n))return OTA_ERR_FLASH;
    }
    r=validate_image(c,OTA_APP_BASE,&c->candidate);if(r)return r;
    next=*c;next.app=c->candidate;memset(&next.candidate,0,sizeof(next.candidate));next.state=OTA_VALID;next.request=0;
    return persist(c,&next);
}
ota_result_t ota_cancel(ota_t*c,uint32_t session) {
    ota_t next;ota_result_t r;
    if(!c || !c->usable)return OTA_ERR_FLASH;
    if(c->state!=OTA_RECEIVING)return OTA_ERR_STATE;
    if(!c->session || c->session!=session)return OTA_ERR_SESSION;
    next=*c;r=zero_image(&c->app)?OTA_ERR_LENGTH:ota_validate_app(c);
    if(r==OTA_ERR_FLASH)return r;
    next.state=r==OTA_OK?OTA_VALID:OTA_EMPTY;if(r!=OTA_OK)memset(&next.app,0,sizeof(next.app));
    memset(&next.candidate,0,sizeof(next.candidate));next.request=0;next.received=0;next.session=0;next.expected_seq=1;next.last_length=0;
    return persist(c,&next);
}
