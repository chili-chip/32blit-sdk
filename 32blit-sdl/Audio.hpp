#include <atomic>

class Audio {
	public:
		Audio();
		~Audio();

		void set_muted(bool muted) { _muted = muted; }
		bool is_muted() const { return _muted; }

	private:
        const unsigned int _sample_rate = 22050;

        SDL_AudioDeviceID audio_device = 0;
        std::atomic<bool> _muted{false};
};
