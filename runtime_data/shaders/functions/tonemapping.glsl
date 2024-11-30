

vec3 gammaCorrect(vec3 color, float gamma) {
	return pow(color, vec3(1.0 / gamma));
}


vec3 tonemapReinhard(vec3 color) {
	return color / (color + vec3(1.0));
}

vec3 tonemapFilmicUncharted2(vec3 color, float exposureBias) {
    // Constants used in the Uncharted 2 tonemapping curve
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;

    // The white point that determines the maximum displayable luminance
    const float W = .2;

    // Uncharted 2 filmic curve
    vec3 mappedColor = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;

    // Normalize by the white point
    vec3 whiteScale = vec3(1.0 / (((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F));
    
    // Apply exposure bias and scale
    return mappedColor * whiteScale * exposureBias;
}

vec3 tonemapACES(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;

    // Apply the ACES filmic tonemapping curve
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}
