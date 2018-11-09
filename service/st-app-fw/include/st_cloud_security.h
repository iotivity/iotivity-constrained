/****************************************************************************
*
* Copyright 2018 Samsung Electronics All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
* either express or implied. See the License for the specific
* language governing permissions and limitations under the License.
*
****************************************************************************/

#ifdef OC_RPK

#ifndef CLOUD_SECURITY_H
#define CLOUD_SECURITY_H

#define JWT_BUFFER_SIZE 512

#ifdef __cplusplus
extern "C" {
#endif

int st_sign_jwt_getter(char **outbuf, const char *pub_key, const char *priv_key, const char *sn);

#ifdef __cplusplus
}
#endif
#endif /*CLOUD_SECURITY_H*/
#endif /*OC_RPK*/
