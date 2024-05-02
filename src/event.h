/*-*- coding:utf-8                                                          -*-│
│vi: set net ft=c ts=4 sts=4 sw=4 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2024 Howard Chu                                                    │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/

#ifndef STAT_H
#define STAT_H

#define MAX_ENTRIES 204800
#define MAX_STACKS 32
#define MAX_EVENTS 1500

#define TP_TRGR(index)                                    \
SEC("tp")                                                 \
int tp_trgr_##index(void* ctx)                            \
{                                                         \
	u64 *cnt, pid_tgid = bpf_get_current_pid_tgid();      \
	pid_t pid = pid_tgid >> 32;                           \
	u32 zero = 0, key = (index);                          \
	if (filter_pid(pid) && !enable)                       \
		return 0;                                         \
	cnt = bpf_map_lookup_insert(&event_cnt, &key, &zero); \
	if (cnt)                                              \
		__sync_fetch_and_add(cnt, 1);                     \
	else                                                  \
		return -1;                                        \
	return 0;                                             \
}                                                         \

struct stack_array {
	unsigned long array[MAX_STACKS];
};

#endif
