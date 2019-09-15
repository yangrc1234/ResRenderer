VertData(pos, 0, vec3);

uniform vec4 _Tint;
INTERP vec4 color;

#ifdef VERTEX
vec4 vertex() {
    color = _Tint * vec4(pos.xyz, 1.0);
    return vec4(pos.xyz, 1.0);
}

#else
vec4 fragment(){
    return color;
}

#endif