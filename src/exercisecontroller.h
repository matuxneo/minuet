/****************************************************************************
**
** Copyright (C) 2015 by Sandro S. Andrade <sandroandrade@kde.org>
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License as
** published by the Free Software Foundation; either version 2 of
** the License or (at your option) version 3 or any later version
** accepted by the membership of KDE e.V. (or its successor approved
** by the membership of KDE e.V.), which shall act as a proxy
** defined in Section 14 of version 3 of the license.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef EXERCISECONTROLLER_H
#define EXERCISECONTROLLER_H

#include <QtCore/QObject>

#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>

class MidiSequencer;

class ExerciseController : public QObject
{
    Q_OBJECT

public:
    ExerciseController(MidiSequencer *midiSequencer);
    virtual ~ExerciseController();
    
    Q_INVOKABLE void setExerciseOptions(QJsonArray exerciseOptions);
    Q_INVOKABLE QString randomlyChooseExercise();
    Q_INVOKABLE unsigned int chosenRootNote();
    Q_INVOKABLE void playChoosenExercise();

    bool configureExercises();
    QString errorString() const;
    QJsonObject exercises() const;

private:
    QJsonArray mergeExercises(QJsonArray exercises, QJsonArray newExercises);

private:
    MidiSequencer *m_midiSequencer;
    QJsonObject m_exercises;
    QJsonArray m_exerciseOptions;
    unsigned int m_chosenExercise;
    unsigned int m_chosenRootNote;
    QString m_errorString;
};

#endif // EXERCISECONTROLLER_H
