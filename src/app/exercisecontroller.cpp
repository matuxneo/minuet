/****************************************************************************
**
** Copyright (C) 2016 by Sandro S. Andrade <sandroandrade@kde.org>
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

#include "exercisecontroller.h"

#include "midisequencer.h"

#include <KLocalizedString>

#include <QDir>
#include <QDateTime>
#include <QJsonDocument>
#include <QStandardPaths>

#include <QtQml> // krazy:exclude=includes

#include <drumstick/alsaevent.h>

ExerciseController::ExerciseController(MidiSequencer *midiSequencer) :
    m_midiSequencer(midiSequencer),
    m_minRootNote(0),
    m_maxRootNote(0),
    m_playMode(ScalePlayMode),
    m_answerLength(1),
    m_chosenRootNote(0),
    m_chosenExercise(0)
{
    qmlRegisterType<ExerciseController>("org.kde.minuet", 1, 0, "ExerciseController");
}

ExerciseController::~ExerciseController()
{
}

void ExerciseController::setExerciseOptions(QJsonArray exerciseOptions)
{
    m_exerciseOptions = exerciseOptions;
}

void ExerciseController::setMinRootNote(unsigned int minRootNote)
{
    m_minRootNote = minRootNote;
}

void ExerciseController::setMaxRootNote(unsigned int maxRootNote)
{
    m_maxRootNote = maxRootNote;
}

void ExerciseController::setPlayMode(PlayMode playMode)
{
    m_playMode = playMode;
}

void ExerciseController::setAnswerLength(unsigned int answerLength)
{
    m_answerLength = answerLength;
}

QStringList ExerciseController::randomlyChooseExercises()
{
    qsrand(QDateTime::currentDateTimeUtc().toTime_t());
    QStringList chosenExercises;

    Song *song = new Song;
    song->setHeader(0, 1, 60);
    song->setInitialTempo(600000);
    m_midiSequencer->setSong(song);
    m_midiSequencer->appendEvent(m_midiSequencer->SMFTempo(600000), 0);

    unsigned int barStart = 0;
    if (m_playMode == RhythmPlayMode) {
        m_midiSequencer->appendEvent(m_midiSequencer->SMFNoteOn(9, 80, 120), 0);
        m_midiSequencer->appendEvent(m_midiSequencer->SMFNoteOn(9, 80, 120), 60);
        m_midiSequencer->appendEvent(m_midiSequencer->SMFNoteOn(9, 80, 120), 120);
        m_midiSequencer->appendEvent(m_midiSequencer->SMFNoteOn(9, 80, 120), 180);
        barStart = 240;
    }

    for (unsigned int i = 0; i < m_answerLength; ++i) {
        m_chosenExercise = qrand() % m_exerciseOptions.size();
        QString sequence = m_exerciseOptions[m_chosenExercise].toObject()[QStringLiteral("sequence")].toString();

        if (m_playMode != RhythmPlayMode) {
            int minNote = INT_MAX;
            int maxNote = INT_MIN;
            foreach(const QString &additionalNote, sequence.split(' ')) {
                int note = additionalNote.toInt();
                if (note > maxNote) maxNote = note;
                if (note < minNote) minNote = note;
            }
            do
                m_chosenRootNote = m_minRootNote + qrand() % (m_maxRootNote - m_minRootNote);
            while (m_chosenRootNote + maxNote > 108 || m_chosenRootNote + minNote < 21);

            m_midiSequencer->appendEvent(m_midiSequencer->SMFNoteOn(1, m_chosenRootNote, 120), barStart);
            m_midiSequencer->appendEvent(m_midiSequencer->SMFNoteOff(1, m_chosenRootNote, 120), barStart + 60);
 
            unsigned int j = 1;
            drumstick::SequencerEvent *ev;
            foreach(const QString &additionalNote, sequence.split(' ')) {
                m_midiSequencer->appendEvent(ev = m_midiSequencer->SMFNoteOn(1,
                                                                   m_chosenRootNote + additionalNote.toInt(),
                                                                   120),
                                                                   (m_playMode == ScalePlayMode) ? barStart+60*j:barStart);
                ev->setTag(0);
                m_midiSequencer->appendEvent(ev = m_midiSequencer->SMFNoteOff(1,
                                                                   m_chosenRootNote + additionalNote.toInt(),
                                                                   120),
                                                                   (m_playMode == ScalePlayMode) ? barStart+60*(j+1):barStart+60);
                ev->setTag(0);
                ++j;
            }
            barStart += 60;
        }
        else {
            m_midiSequencer->appendEvent(m_midiSequencer->SMFNoteOn(9, 80, 120), barStart);
            foreach(QString additionalNote, sequence.split(' ')) { // krazy:exclude=foreach
                m_midiSequencer->appendEvent(m_midiSequencer->SMFNoteOn(9, 37, 120), barStart);
                float dotted = 1;
                if (additionalNote.endsWith('.')) {
                    dotted = 1.5;
                    additionalNote.chop(1);
                }
                barStart += dotted*60*(4.0/additionalNote.toInt());
            }
        }

        chosenExercises << m_exerciseOptions[m_chosenExercise].toObject()[QStringLiteral("name")].toString();
    }
    if (m_playMode == RhythmPlayMode) {
        m_midiSequencer->appendEvent(m_midiSequencer->SMFNoteOn(9, 80, 120), barStart);
    }

    return chosenExercises;
}

unsigned int ExerciseController::chosenRootNote()
{
    return m_chosenRootNote;
}

void ExerciseController::playChoosenExercise()
{
    m_midiSequencer->play();
}

bool ExerciseController::configureExercises()
{
    m_errorString.clear();
    QStringList exercisesDirs = QStandardPaths::locateAll(QStandardPaths::AppDataLocation, QStringLiteral("exercises"), QStandardPaths::LocateDirectory);
    foreach (const QString &exercisesDirString, exercisesDirs) {
        QDir exercisesDir(exercisesDirString);
        foreach (const QString &exercise, exercisesDir.entryList(QDir::Files)) {
            QFile exerciseFile(exercisesDir.absoluteFilePath(exercise));
            if (!exerciseFile.open(QIODevice::ReadOnly)) {
                m_errorString = i18n("Couldn't open exercise file \"%1\".", exercisesDir.absoluteFilePath(exercise));
                return false;
            }
            QJsonParseError error;
            QJsonDocument jsonDocument = QJsonDocument::fromJson(exerciseFile.readAll(), &error);

            if (error.error != QJsonParseError::NoError) {
                m_errorString = error.errorString();
                exerciseFile.close();
                return false;
            }
            else {
                if (m_exercises.length() == 0)
                    m_exercises = jsonDocument.object();
                else
                    m_exercises[QStringLiteral("exercises")] = mergeExercises(m_exercises[QStringLiteral("exercises")].toArray(),
                                                            jsonDocument.object()[QStringLiteral("exercises")].toArray());
            }
            exerciseFile.close();
        }
    }
    return true;
}

QString ExerciseController::errorString() const
{
    return m_errorString;
}

QJsonObject ExerciseController::exercises() const
{
    return m_exercises;
}

QJsonArray ExerciseController::mergeExercises(QJsonArray exercises, QJsonArray newExercises)
{
    for (QJsonArray::ConstIterator i1 = newExercises.constBegin(); i1 < newExercises.constEnd(); ++i1) {
        if (i1->isObject()) {
            QJsonArray::ConstIterator i2;
            for (i2 = exercises.constBegin(); i2 < exercises.constEnd(); ++i2) {
                if (i2->isObject() && i1->isObject() && i2->toObject()[QStringLiteral("name")] == i1->toObject()[QStringLiteral("name")]) {
                    QJsonObject jsonObject = exercises[i2-exercises.constBegin()].toObject();
                    jsonObject[QStringLiteral("children")] = mergeExercises(i2->toObject()[QStringLiteral("children")].toArray(), i1->toObject()[QStringLiteral("children")].toArray());
                    exercises[i2-exercises.constBegin()] = jsonObject;
                    break;
                }
            }
            if (i2 == exercises.constEnd())
                exercises.append(*i1);
        }
    }
    return exercises;
}
