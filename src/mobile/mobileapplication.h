/* Copyright (C) 2004-2022 Robert Griebl. All rights reserved.
**
** This file is part of BrickStore.
**
** This file may be distributed and/or modified under the terms of the GNU
** General Public License version 2 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://fsf.org/licensing/licenses/gpl.html for GPL licensing information.
*/
#pragma once

#include <QVariantMap>

#include "common/application.h"


class MobileApplication : public Application
{
    Q_OBJECT

public:
    MobileApplication(int &argc, char **argv);
    ~MobileApplication() override;

    void init() override;

protected:
    QCoro::Task<bool> closeAllDocuments() override;
    void setupQml() override;
    void setupLogging() override;

private:
    QVariantMap m_actionMap;
};
