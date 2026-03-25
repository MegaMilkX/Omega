#pragma once

#include <assert.h>

enum extension {
    e_ext_unknown,
    // text
    e_txt,
    e_json,
    e_xml,
    e_ini,
    e_cfg,
    e_csv,
    // image
    e_png,
    e_jpg,
    e_jpeg,
    e_gif,
    e_bmp,
    e_dds,
    e_tiff,
    e_tga,
    // material
    e_mat,
    e_material,
    // animation
    e_anim,
    e_animation,
    // sound
    e_ogg,
    e_wav,
    // 3d model
    e_m3d,
    e_fbx, 
    e_obj,
    // intermediate import definitions
    e_m3dp,
    // other
    e_apf,              // actor prefab
    e_skeletal_model,
    e_static_model,
    e_skl,
    e_skeleton,
    // shaders
    e_glsl,
};
inline const char* extension_to_string(extension e) {
    switch (e) {
    // text
    case e_txt: return "txt";
    case e_json: return "json";
    case e_xml: return "xml";
    case e_ini: return "ini";
    case e_cfg: return "cfg";
    case e_csv: return "csv";
    // image
    case e_png: return "png";
    case e_jpg: return "jpg";
    case e_jpeg: return "jpeg";
    case e_gif: return "gif";
    case e_bmp: return "bmp";
    case e_dds: return "dds";
    case e_tiff: return "tiff";
    case e_tga: return "tga";
    // material
    case e_mat: return "mat";
    case e_material: return "material";
    // animation
    case e_anim: return "anim";
    case e_animation: return "animation";
    // sound
    case e_ogg: return "ogg";
    case e_wav: return "wav";
    // 3d model sources
    case e_m3d: return "m3d";
    case e_fbx: return "fbx";
    case e_obj: return "obj";
    // intermediate import definitions
    case e_m3dp: return "m3dp";
    // other
    case e_apf: return "apf"; // actor prefab
    case e_skeletal_model: return "skeletal_model";
    case e_static_model: return "static_model";
    case e_skl: return "skl";
    case e_skeleton: return "skeleton";

    case e_glsl: return "glsl";
    };

    assert(false);
    return "";
}

