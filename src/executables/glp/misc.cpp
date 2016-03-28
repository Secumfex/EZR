
#include <Rendering/GLTools.h>
#include <algorithm>

inline void computeMasks(int level, std::vector<std::vector<GLfloat>>& binomialMasks)
{
	binomialMasks.resize(level+1);

	binomialMasks[0].resize(1); // base level
	binomialMasks[0][0] = 1;

	for ( int i = 1; i <= level; i++ )
	{
		binomialMasks[i].resize(i + 1);

		for ( int j = 0; j < binomialMasks[i].size();j ++)
		{
			// left Value
			int left = 0;
			if (j-1 >= 0 && j-1 < binomialMasks[i-1].size())
			{
				left = binomialMasks[i-1][j-1]; 
			}
			int right = 0;
			if (j >= 0 && j < binomialMasks[i-1].size())
			{
				right = binomialMasks[i-1][j]; 
			}
			binomialMasks[i][j] = left + right;
		}
	}

	// normalize each level
	for ( int i = 1; i <= level; i++ )
	{
		GLfloat weight = 0.0f;
		for ( int j = 0; j < binomialMasks[i].size(); j++)
		{
			weight += binomialMasks[i][j];
		}
		for ( int j = 0; j < binomialMasks[i].size(); j++)
		{
			binomialMasks[i][j] /= weight;
		}
	}
}

inline float log_2( float n )  
{  
    return log( n ) / log( 2 );
}