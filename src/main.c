/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "App.h"
			

int main(void)
{
	App_Init();

	while (1)
	{
		App_Process();
	}
}
