uniform etLowp vec4 linesColor;

etFragmentIn etLowp vec4 aColor;

void main()
{
	etFragmentOut = aColor * linesColor;
}