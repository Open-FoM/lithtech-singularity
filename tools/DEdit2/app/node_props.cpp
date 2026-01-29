#include "node_props.h"

NodeProperties MakeProps(const char* type) {
  NodeProperties props;
  props.type = type;
  if (props.type == "World") {
    props.gravity = -9.8f;
    props.fog_density = 0.005f;
  } else if (props.type == "Light") {
    props.intensity = 5.0f;
    props.range = 25.0f;
    props.cast_shadows = true;
  } else if (props.type == "Terrain") {
    props.height_scale = 15.0f;
  } else if (props.type == "Texture") {
    props.srgb = true;
    props.mipmaps = true;
    props.compression_mode = 1;
  } else if (props.type == "Sound") {
    props.volume = 0.8f;
    props.loop = false;
  }
  return props;
}
