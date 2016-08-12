/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#ifndef OC_STORE_H_
#define OC_STORE_H_

void oc_sec_load_pstat(void);
void oc_sec_load_doxm(void);
void oc_sec_load_cred(void);
void oc_sec_load_acl(void);
void oc_sec_dump_state(void);

#endif /* OC_STORE_H_ */
