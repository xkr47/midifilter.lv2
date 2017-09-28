MFD_FILTER(latch)

#ifdef MX_TTF

	mflt:latch
	TTF_DEFAULTDEF("MIDI Latch", "MIDI Latch")
	, TTF_IPORT(0, "channelf", "Filter Channel", 0, 16, 0,
			PORTENUMZ("Any")
			DOC_CHANF)
	, TTF_IPORT( 1, "threshold",  "New chord threshold [sec]", 0.0, 10.0,  1.0, units:unit units:s ;
			rdfs:comment "If this much time has passed when key is pressed, a new chord is started.")
	; rdfs:comment "This filter delays note-off messages until new chord is started. Current chord ends when idle time passes without keypresses."
	.

#elif defined MX_CODE

void
filter_midi_latch(MidiFilter* self,
		const uint32_t tme,
		const uint8_t* const buffer,
		const uint32_t size)
{
	const uint8_t chs = midi_limit_chn(floorf(*self->cfg[0]) -1);
	const uint8_t chn = buffer[0] & 0x0f;
	const uint8_t mst = buffer[0] & 0xf0;

	if (size != 3
			|| !(mst == MIDI_NOTEON || mst == MIDI_NOTEOFF)
			|| !(floorf(*self->cfg[0]) == 0 || chs == chn)
		 )
	{
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	const uint8_t key = buffer[1] & 0x7f;
	const uint8_t vel = buffer[2] & 0x7f;

	if (mst == MIDI_NOTEOFF || (mst == MIDI_NOTEON && vel ==0)) {
		return;
	}

	const uint32_t lastT = (uint32_t) self->memI[0];

	struct timeval tv;
	gettimeofday(&tv, NULL);
	uint32_t t = tv.tv_sec * 1000000 + tv.tv_usec;
	
	if (t - lastT > 1000000 * RAIL((*self->cfg[1]), 0, 120)) {
		// shut down all current notes
		int k;
		for (k=0; k < 127; ++k) {
			if (self->memCI[chn][k]) {
				uint8_t buf[3];
				buf[0] = MIDI_NOTEOFF | chn;
				buf[1] = k;
				buf[2] = 0;
				forge_midimessage(self, tme, buf, 3);
				self->memCI[chn][k] = 0;
			}
		}
	}

	self->memI[0] = (int) t;

	if (self->memCI[chn][key]) {
		uint8_t buf[3];
		buf[0] = MIDI_NOTEOFF | chn;
		buf[1] = key;
		buf[2] = 0;
		forge_midimessage(self, tme, buf, 3);
	}
	forge_midimessage(self, tme, buffer, size);
	self->memCI[chn][key] = 1;
	       
}

static void filter_init_latch(MidiFilter* self) {
	int c,k;
	for (c=0; c < 16; ++c) for (k=0; k < 127; ++k) {
		self->memCI[c][k] = 0;
	}
}

#endif
