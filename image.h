#ifndef LUMA_IMAGE_H
#define LUMA_IMAGE_H

namespace luma
{
	class Image
	{
	private:
		const std::int32_t _width, _height;
		std::uint32_t* _data;

	public:
		Image(std::int32_t, std::int32_t, bool) noexcept;

	public:
		void shadow_from(std::uint32_t*) noexcept;
		void export_to_ppm(const std::string&) noexcept;
		void export_to_bmp(const std::string&) noexcept;
	};
}

#endif
