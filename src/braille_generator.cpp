//
// Created by adeleep on 06/06/2023.
//
#include "braille_generator.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

using namespace Braille_Generator;

std::vector<std::string> Braille_Generator::generate_braille(int width_w, int height_w, const char *filename) { // width_w divisible by 2; height_w divisible by 4
    int width, height, bpp;

    uint8_t* rgb_image = stbi_load(filename, &width, &height, &bpp, 1);

    uint8_t* rgb_image_w;

    rgb_image_w = static_cast<uint8_t *>(malloc(width_w * height_w * 1));

    stbir_resize_uint8(rgb_image, width, height, 0,
                       rgb_image_w, width_w, height_w, 0,
                       1);

    stbi_image_free(rgb_image); // free original

    auto* dithered = static_cast<uint8_t *>(malloc(width_w * height_w * 1));

    for ( int i = 0; i < height_w; i++ ) {
        for (int j =0; j < width_w; j++){
            *(dithered + (i*width_w + j)) = *(rgb_image_w + (i*width_w + j)) < 128 ? 0 : 255 ;
            auto error_diffuse = (*(rgb_image_w + (i*width_w + j)) - *(dithered + (i*width_w + j)))/16.0;

            if (j < width_w-1){ // right col exists
                *(rgb_image_w + (i*width_w + j+1)) += (uint8_t) error_diffuse*7;
            }
            if (i < height_w-1){ //bottom row exists
                *(rgb_image_w + ((i+1)*width_w + j)) += (uint8_t) error_diffuse*5;

                if (j != 0){ // left col exists
                    *(rgb_image_w + ((i+1)*width_w + j-1)) += (uint8_t) error_diffuse*3;
                }
                if (j < width_w-1){ // right col exists
                    *(rgb_image_w + ((i+1)*width_w + j+1)) += (uint8_t) error_diffuse*1;
                }
            }
        }
    }

    free(rgb_image_w); // free resized

    std::vector<std::string> out;
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    for ( int i = 0; i < height_w; i+=4 ) {
       std::string line;
        for (int j =0; j < width_w; j+=2){
            std::string bits;
            for (int braille_w = 0; braille_w<2;++braille_w){ // 1, 2, 3, 4, 5, 6
                for (int braille_h = 0; braille_h<3;++braille_h){
                    if (*(dithered + ((i+braille_h)*width_w + j+braille_w)) == 0){
                        bits.push_back('1');
                    } else{
                        bits.push_back('0');
                    }
                }
            }
            if (*(dithered + ((i+3)*width_w + j+0)) == 0){ // 7
                bits.push_back('1');
            } else{
                bits.push_back('0');
            }
            if (*(dithered + ((i+3)*width_w + j+1)) == 0){ // 8
                bits.push_back('1');
            } else{
                bits.push_back('0');
            }

            std::reverse(bits.begin(), bits.end());

            std::bitset<24>  offset(bits);
            std::bitset<24>  val((int)(offset.to_ulong()) + 10240);
            std::stringstream ss;
            ss << std::hex << std::uppercase  << val.to_ulong();
            std::string braille_symbol =  ss.str();

            int code_point = std::stoi (braille_symbol, nullptr, 16);
            std::wstring wide_str = {static_cast<wchar_t>(code_point)};
            std::string utf8_str = converter.to_bytes(wide_str);


            line.append(utf8_str);
        }
        out.push_back(line);
    }

    free(dithered);

    return out;
}

std::vector<std::vector<std::string>> Braille_Generator::generate_all_frames(int width, int height, const std::string& directory, int frame_count) {
   std::vector<std::vector<std::string>> frames;
    for (int i = 0; i<frame_count; ++i){
        std::string filename = directory+std::to_string(i)+".png";
        std::vector<std::string> out = generate_braille(width,height,filename.c_str());
        frames.push_back(out);
    }
    return frames;
}

#define NUMBER_OF_FRAMES 10
#define ANIMATION_DIR "Tuna/"
