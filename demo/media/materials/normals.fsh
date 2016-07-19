etFragmentIn vec3 vNormalWS;

void main()
{
    vec3 vNormal = normalize(vNormalWS);
    etFragmentOut = vec4(0.5 + 0.5 * vNormal, 1.0);
}