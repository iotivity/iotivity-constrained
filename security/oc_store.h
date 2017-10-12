/*
// Copyright (c) 2017 Intel Corporation
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

#ifndef OC_STORE_H
#define OC_STORE_H

void oc_sec_load_pstat(int device);
void oc_sec_load_doxm(int device);
void oc_sec_load_cred(int device);
void oc_sec_load_acl(int device);
void oc_sec_dump_pstat(int device);
void oc_sec_dump_cred(int device);
void oc_sec_dump_doxm(int device);
void oc_sec_dump_acl(int device);
void oc_sec_dump_unique_ids(int device);
void oc_sec_load_unique_ids(int device);

#endif /* OC_STORE_H */
