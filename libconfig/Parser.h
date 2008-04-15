#ifndef PARSER_H
#	define PARSER_H

#include "Data.h"
#include "String.h"

#include <Object.h>

@interface Parser: Object
-free;
-init;
-(DataObject *)getDataFrom: (String *)line;
-(int)getLineLenFrom: (const char *)startAddr to: (const char *)endAddr;
@end
#endif
