/*=========================================================================

   Program: ParaView
   Module:    pqSaveSnapshotDialog.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "pqSaveSnapshotDialog.h"
#include "ui_pqSaveSnapshotDialog.h"

// Server Manager Includes.

// Qt Includes.
#include <QIntValidator>

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqSettings.h"
#include "pqStereoModeHelper.h"
#include "vtkPVProxyDefinitionIterator.h"
#include "vtkRenderWindow.h" // for VTK_STEREO_*
#include "vtkSMProxy.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

#define SETTINGS_KEY "pqSaveSnapshotDialog/SaveOnlySelectedView"

class pqSaveSnapshotDialog::pqInternal : public Ui::SaveSnapshotDialog
{
public:
  double AspectRatio;
  QSize ViewSize;
  QSize AllViewsSize;
};

//-----------------------------------------------------------------------------
pqSaveSnapshotDialog::pqSaveSnapshotDialog(QWidget* _parent, Qt::WindowFlags f)
  : Superclass(_parent, f)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->Internal->stereoMode->addItems(pqStereoModeHelper::availableStereoModes());

  this->Internal->AspectRatio = 1.0;
  this->Internal->quality->setMinimum(0);
  this->Internal->quality->setMaximum(100);
  this->Internal->quality->setValue(100);

  QIntValidator* validator = new QIntValidator(this);
  validator->setBottom(50);
  this->Internal->width->setValidator(validator);

  validator = new QIntValidator(this);
  validator->setBottom(50);
  this->Internal->height->setValidator(validator);

  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings && settings->contains(SETTINGS_KEY))
  {
    this->Internal->selectedViewOnly->setChecked(settings->value(SETTINGS_KEY).toBool());
  }

  QObject::connect(this->Internal->width, SIGNAL(editingFinished()), this, SLOT(onWidthEdited()));
  QObject::connect(this->Internal->height, SIGNAL(editingFinished()), this, SLOT(onHeightEdited()));
  QObject::connect(
    this->Internal->lockAspect, SIGNAL(toggled(bool)), this, SLOT(onLockAspectRatio(bool)));
  QObject::connect(
    this->Internal->selectedViewOnly, SIGNAL(toggled(bool)), this, SLOT(updateSize()));

  vtkSMSessionProxyManager* pxm =
    vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager();
  if (pxm->GetProxyDefinitionManager())
  {
    vtkPVProxyDefinitionIterator* iter =
      pxm->GetProxyDefinitionManager()->NewSingleGroupIterator("palettes");
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      vtkSMProxy* prototype = pxm->GetPrototypeProxy("palettes", iter->GetProxyName());
      if (prototype)
      {
        this->Internal->palette->addItem(prototype->GetXMLLabel(), prototype->GetXMLName());
      }
    }
    iter->Delete();
  }
}

//-----------------------------------------------------------------------------
pqSaveSnapshotDialog::~pqSaveSnapshotDialog()
{
  pqSettings* settings =
    pqApplicationCore::instance() ? pqApplicationCore::instance()->settings() : NULL;
  if (settings)
  {
    settings->setValue(SETTINGS_KEY, !this->saveAllViews());
  }

  delete this->Internal;
}

//---------------------------------------------------------------------------
void pqSaveSnapshotDialog::setEnableSaveAllViews(bool enable)
{
  this->Internal->selectedViewOnly->setVisible(enable);
}

//-----------------------------------------------------------------------------
void pqSaveSnapshotDialog::setViewSize(const QSize& view_size)
{
  this->Internal->ViewSize = view_size;
  this->updateSize();
}

//-----------------------------------------------------------------------------
void pqSaveSnapshotDialog::setAllViewsSize(const QSize& view_size)
{
  this->Internal->AllViewsSize = view_size;
  this->updateSize();
}

//-----------------------------------------------------------------------------
bool pqSaveSnapshotDialog::saveAllViews() const
{
  return (!this->Internal->selectedViewOnly->isChecked());
}

//-----------------------------------------------------------------------------
void pqSaveSnapshotDialog::updateSize()
{
  if (this->saveAllViews())
  {
    this->Internal->width->setText(QString::number(this->Internal->AllViewsSize.width()));
    this->Internal->height->setText(QString::number(this->Internal->AllViewsSize.height()));
  }
  else
  {
    this->Internal->width->setText(QString::number(this->Internal->ViewSize.width()));
    this->Internal->height->setText(QString::number(this->Internal->ViewSize.height()));
  }

  QSize curSize = this->viewSize();
  this->Internal->AspectRatio = curSize.width() / static_cast<double>(curSize.height());
}

//-----------------------------------------------------------------------------
void pqSaveSnapshotDialog::onLockAspectRatio(bool lock)
{
  if (lock)
  {
    QSize curSize = this->viewSize();
    this->Internal->AspectRatio = curSize.width() / static_cast<double>(curSize.height());
  }
}

//-----------------------------------------------------------------------------
int pqSaveSnapshotDialog::quality() const
{
  return this->Internal->quality->value();
}

//-----------------------------------------------------------------------------
QSize pqSaveSnapshotDialog::viewSize() const
{
  return QSize(this->Internal->width->text().toInt(), this->Internal->height->text().toInt());
}

//-----------------------------------------------------------------------------
void pqSaveSnapshotDialog::onWidthEdited()
{
  if (this->Internal->lockAspect->isChecked())
  {
    this->Internal->height->setText(QString::number(
      static_cast<int>(this->Internal->width->text().toInt() / this->Internal->AspectRatio)));
  }
}

//-----------------------------------------------------------------------------
void pqSaveSnapshotDialog::onHeightEdited()
{
  if (this->Internal->lockAspect->isChecked())
  {
    this->Internal->width->setText(QString::number(
      static_cast<int>(this->Internal->height->text().toInt() * this->Internal->AspectRatio)));
  }
}

//-----------------------------------------------------------------------------
QString pqSaveSnapshotDialog::palette() const
{
  QString paletteData =
    this->Internal->palette->itemData(this->Internal->palette->currentIndex()).toString();
  return paletteData;
}

//-----------------------------------------------------------------------------
int pqSaveSnapshotDialog::getStereoMode() const
{
  return pqStereoModeHelper::stereoMode(this->Internal->stereoMode->currentText());
}
