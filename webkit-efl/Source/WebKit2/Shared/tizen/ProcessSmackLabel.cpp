/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ProcessSmackLabel.h"

#if ENABLE(TIZEN_PROCESS_PERMISSION_CONTROL)

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/smack.h>
#include <sys/capability.h>
#include <wtf/Assertions.h>

namespace WebKit {

bool changeProcessSmackLabel(const char* defaultExecutablePath, const char* currentExecutablePath)
{
    ASSERT(defaultExecutablePath && currentExecutablePath);

    // this case needs not to change smack label
    if (!strcmp(defaultExecutablePath, currentExecutablePath))
        return true;

    // check if this process is launched as abnormal way
    char* newLabel;
    if (smack_lgetlabel(currentExecutablePath, &newLabel, SMACK_LABEL_EXEC) < 0)
        return false;

    if (smack_set_label_for_self(newLabel) < 0) {
        free(newLabel);
        return false;
    }

    free(newLabel);
    return true;
}

bool dropProcessCapability()
{
    // in case of root user, any capabilities aren't dropped
    if (getuid() == 0)
        return true;

    cap_user_header_t header;
    cap_user_data_t data;

    header = static_cast<cap_user_header_t>(malloc(sizeof(*header)));
    data = static_cast<cap_user_data_t>(calloc(sizeof(*data), _LINUX_CAPABILITY_U32S_3));

    // check if header and data is allocated normally
    ASSERT(header && data);

    header->pid = getpid();
    header->version = _LINUX_CAPABILITY_VERSION_3;

    // read already granted capabilities of this process
    if (capget(header, data) < 0) {
        free(header);
        free(data);
        return false;
    }

    // remove process capability for CAP_MAC_ADMIN
    data[CAP_TO_INDEX(CAP_MAC_ADMIN)].inheritable &= ~CAP_TO_MASK(CAP_MAC_ADMIN);
    data[CAP_TO_INDEX(CAP_MAC_ADMIN)].permitted &= ~CAP_TO_MASK(CAP_MAC_ADMIN);
    data[CAP_TO_INDEX(CAP_MAC_ADMIN)].effective &= ~CAP_TO_MASK(CAP_MAC_ADMIN);

    bool ret = true;
    if (capset(header, data) < 0)
        ret = false;

    free(header);
    free(data);

    return ret;
}

} // namespace WebKit
#endif
