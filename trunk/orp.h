//////////////////////////////////////////////////////////////////////////////
//
// Open Remote Play
// http://ps3-hacks.com
//
//////////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but 
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _ORP_H
#define _ORP_H

using namespace std;

#include <curl/curl.h>

#include <openssl/aes.h>

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_net.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
};

#include <string>
#include <vector>
#include <queue>

#include "config.h"

#define ORP_PREMO_VMAJOR	0
#define ORP_PREMO_VMINOR	3

#define ORP_FRAME_WIDTH		480
#define ORP_FRAME_HEIGHT	272
#define ORP_SESSION_LEN		16
#define ORP_PADSTATE_MAX	60
#define ORP_PADSTATE_LEN	128

#define ORP_USER_AGENT		"premo/1.0.0 libhttp/1.0.0"

#define ORP_GET_SESSION		"/sce/premo/session"
#define ORP_GET_CTRL		"/sce/premo/session/ctrl"
#define ORP_GET_VIDEO		"/sce/premo/session/video"
#define ORP_GET_AUDIO		"/sce/premo/session/audio"
#define ORP_POST_PAD		"/sce/premo/session/pad"

// PSP PAD offsets and bits
#define ORP_PAD_PSP_HOME	0x0101
#define ORP_PAD_PSP_SELECT	0x0504
#define ORP_PAD_PSP_START	0x0508
#define ORP_PAD_PSP_DPUP	0x0510
#define ORP_PAD_PSP_DPRIGHT	0x0520
#define ORP_PAD_PSP_DPDOWN	0x0540
#define ORP_PAD_PSP_DPLEFT	0x0580
#define ORP_PAD_PSP_L1		0x0704
#define ORP_PAD_PSP_R1		0x0708
#define ORP_PAD_PSP_TRI		0x0710
#define ORP_PAD_PSP_CIRCLE	0x0720
#define ORP_PAD_PSP_X		0x0740
#define ORP_PAD_PSP_SQUARE	0x0780
#define ORP_PAD_PSP_YAXIS	0x0e
#define ORP_PAD_PSP_XAXIS	0x10

#define ORP_PAD_PS3_RYAXIS	0x09
#define ORP_PAD_PS3_RXAXIS	0x0b
#define ORP_PAD_PS3_LYAXIS	0x0d
#define ORP_PAD_PS3_LXAXIS	0x0f

#define ORP_PAD_TIMESTAMP	0x40
#define ORP_PAD_EVENTID		0x48

#define ORP_PAD_KEYUP		0x10000000
#define ORP_PAD_KEYDOWN		0x20000000

#define ORP_SRCH_REPLIES	5
#define ORP_SRCH_TIMEOUT	16

// UDP broadcast from PSP to *:9293
#define ORP_ANNOUNCE_SRCH	"SRCH"

struct PktAnnounceSrch_t {
	Uint8 id[4];
};

#define CreatePktAnnounceSrch(pkt) \
	memcpy(pkt.id, ORP_ANNOUNCE_SRCH, 4);

// UDP reply from PlayStation 3, 156 bytes
#define ORP_ANNOUNCE_RESP	"RESP"

struct PktAnnounceResp_t {
	Uint8 id[4];				// 4
	Uint8 unk0[6];				// 10
	Uint8 ps3_mac[ORP_MAC_LEN];	// 16
	Uint8 ps3_nickname[ORP_NICKNAME_LEN];
								// 144
#define ORP_PS3_NPX_LEN		12
	Uint8 npx[ORP_PS3_NPX_LEN];
								// 156
};

enum orpHeader {
	HEADER_NULL,

	HEADER_AUDIO_BITRATE,
	HEADER_AUDIO_CHANNELS,
	HEADER_AUDIO_CLOCKFREQ,
	HEADER_AUDIO_CODEC,
	HEADER_AUDIO_SAMPLERATE,
	HEADER_AUTH,
	HEADER_CTRL_BITRATE,
	HEADER_CTRL_MAXBITRATE,
	HEADER_CTRL_MODE,
	HEADER_EXEC_MODE,
	HEADER_MODE,
	HEADER_NONCE,
	HEADER_PAD_ASSIGN,
	HEADER_PAD_COMPLETE,
	HEADER_PAD_INFO,
	HEADER_PLATFORM_INFO,
	HEADER_POWER_CONTROL,
	HEADER_PS3_NICKNAME,
	HEADER_PSPID,
	HEADER_SESSIONID,
	HEADER_TRANS,
	HEADER_TRANS_MODE,
	HEADER_USERNAME,
	HEADER_VERSION,
	HEADER_VIDEO_BITRATE,
	HEADER_VIDEO_CLOCKFREQ,
	HEADER_VIDEO_CODEC,
	HEADER_VIDEO_FRAMERATE,
	HEADER_VIDEO_RESOLUTION,
};

struct orpHeader_t {
	enum orpHeader header;
	string name;
};

struct orpHeaderValue_t {
	enum orpHeader header;
	string value;
};

enum orpCtrlMode {
	CTRL_CHANGE_BITRATE,
	CTRL_SESSION_TERM,

	CTRL_NULL,
};

struct orpCtrlMode_t {
	enum orpCtrlMode mode;
	string param1;
	string param2;
};

enum orpEvent {
	EVENT_ERROR,
	EVENT_RESTORE,
	EVENT_SHUTDOWN
};

enum orpViewSize {
	VIEW_NORMAL,
	VIEW_MEDIUM,
	VIEW_LARGE,
	VIEW_FULLSCREEN
};

struct orpView_t {
	enum orpViewSize size;
	enum orpViewSize prev;
	SDL_Surface *view;
	SDL_Overlay *overlay;
	SDL_Rect scale;
	SDL_mutex *viewLock;
};

struct orpKey_t {
	Uint8 skey0[ORP_KEY_LEN];
	Uint8 skey1[ORP_KEY_LEN];
	Uint8 skey2[ORP_KEY_LEN];
	Uint8 psp_id[ORP_KEY_LEN];
	Uint8 pkey[ORP_KEY_LEN];
	Uint8 xor_pkey[ORP_KEY_LEN];
	Uint8 nonce[ORP_KEY_LEN];
	Uint8 xor_nonce[ORP_KEY_LEN];
	Uint8 akey[ORP_KEY_LEN];
	Uint8 iv1[ORP_KEY_LEN];
};

struct orpConfig_t {
	Uint8 ps3_mac[ORP_MAC_LEN];
	char psp_owner[ORP_NICKNAME_LEN];
	Uint8 psp_mac[ORP_MAC_LEN];
	char ps3_addr[ORP_HOSTNAME_LEN];
	Uint16 ps3_port;
	bool ps3_discover;
	struct orpKey_t key;
};

struct orpStreamPacketHeader_t {
	Uint8 magic[2];	// 2
	Uint16 frame;	// 4
	Uint32 clock;	// 8
	Uint8 root[4];	// 12
	Uint16 unk2;	// 14
	Uint16 unk3;	// 16
	Uint16 len;		// 18
	Uint16 unk4;	// 20
	Uint16 unk5;	// 22
	Uint16 unk6;	// 24
	Uint16 unk7;	// 26
	Uint16 unk8;	// 28
	Uint16 unk9;	// 30
	Uint16 unk10;	// 32
};

struct orpStreamPacket_t {
	struct orpStreamPacketHeader_t header;
	Uint32 len;
	Uint8 *data;
};

struct orpStreamData_t {
	Uint32 len;
	Uint32 pos;
	Uint8 *data;
	Uint32 frames;
	SDL_mutex *packetLock;
	SDL_cond *packetCond;
	queue<struct orpStreamPacket_t *> packetList;
};

struct orpThreadDecode_t {
	bool terminate;
	AVCodec *codec;
	struct orpView_t *view;
	struct orpStreamData_t *stream;
};

struct orpConfigStream_t {
	string name;
	string url;
	string codec;
	string session_id;
	AES_KEY aes_key;
	struct orpKey_t *key;
	struct orpStreamData_t *stream;
};

struct orpAudioFrame_t {
	Uint32 len;
	Uint8 *data;
};

struct orpConfigAudioFeed_t {
	SDL_mutex *feedLock;
	queue<struct orpAudioFrame_t *> frameList;
};

struct orpCodec_t {
	string name;
	AVCodec *codec;
};

class OpenRemotePlay
{
public:
	OpenRemotePlay(struct orpConfig_t *config);
	~OpenRemotePlay();

	bool SessionCreate(void);
	void SessionDestroy(void);

protected:
	struct orpConfig_t config;
	vector<struct orpCodec_t *> codec;
	struct orpView_t view;
	string session_id;
	char *ps3_nickname;
	SDL_Thread *thread_video_connection;
	SDL_Thread *thread_video_decode;
	SDL_Thread *thread_audio_connection;
	SDL_Thread *thread_audio_decode;

	bool CreateView(void);
	bool CreateKeys(const string &nonce);
	AVCodec *GetCodec(const string &name);
	Sint32 ControlPerform(CURL *curl, struct orpCtrlMode_t *mode);
	Sint32 SessionControl(void);
	Sint32 SessionPerform(void);
};

#endif // _ORP_H
// vi: ts=4