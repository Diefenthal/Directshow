#ifndef H_MATH
#define H_MATH

void ReduceFraction(UINT &Num, UINT &Denom)
{
        UINT a = 0;
        UINT b = 0;
        UINT i = 0;
        a = Denom;
        b = Num;
		
        for (i = 50; i > 1; i--)
        {
                if ((a % i == 0) && (b % i == 0))
                {
                        a /= i;
                        b /= i;
                }
        }

        Denom = a;
        Num = b;
}

#endif