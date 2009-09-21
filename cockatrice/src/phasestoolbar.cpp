#include "phasestoolbar.h"
#include <QAction>
#include <QVBoxLayout>
#include <QPainter>
#include <QPen>

PhaseButton::PhaseButton(const QIcon &icon, QAction *_doubleClickAction)
	: QPushButton(icon, QString()), active(false), doubleClickAction(_doubleClickAction), activePixmap(50, 50), inactivePixmap(50, 50)
{
	setFixedSize(50, 50);
	
	updatePixmap(activePixmap, true);
	updatePixmap(inactivePixmap, false);
}

void PhaseButton::updatePixmap(QPixmap &pixmap, bool active)
{
	pixmap.fill(Qt::transparent);
	
	QPainter painter(&pixmap);
	int height = pixmap.height();
	int width = pixmap.width();

	if (active)
		painter.setBrush(Qt::red);
	painter.setPen(Qt::gray);
	painter.drawRect(1, 1, width - 2, height - 2);

	icon().paint(&painter, 5, 5, width - 10, height - 10);
}

void PhaseButton::paintEvent(QPaintEvent */*event*/)
{
	QPainter painter(this);
	if (active)
		painter.drawPixmap(0, 0, size().width(), size().height(), activePixmap);
	else
		painter.drawPixmap(0, 0, size().width(), size().height(), inactivePixmap);
}

void PhaseButton::setPhaseText(const QString &_phaseText)
{
	phaseText = _phaseText;
	setToolTip(phaseText);
}

void PhaseButton::mouseDoubleClickEvent(QMouseEvent */*event*/)
{
	if (doubleClickAction)
		doubleClickAction->trigger();
}

PhasesToolbar::PhasesToolbar(QWidget *parent)
	: QFrame(parent)
{
	QAction *aUntapAll = new QAction(this);
	connect(aUntapAll, SIGNAL(triggered()), this, SIGNAL(signalUntapAll()));
	QAction *aDrawCard = new QAction(this);
	connect(aDrawCard, SIGNAL(triggered()), this, SIGNAL(signalDrawCard()));
	
	PhaseButton *untapButton = new PhaseButton(QIcon(":/resources/icon_phase_untap.svg"), aUntapAll);
	PhaseButton *upkeepButton = new PhaseButton(QIcon(":/resources/icon_phase_upkeep.svg"));
	PhaseButton *drawButton = new PhaseButton(QIcon(":/resources/icon_phase_draw.svg"), aDrawCard);
	PhaseButton *main1Button = new PhaseButton(QIcon(":/resources/icon_phase_main1.svg"));
	PhaseButton *combatStartButton = new PhaseButton(QIcon(":/resources/icon_phase_combat_start.svg"));
	PhaseButton *combatAttackersButton = new PhaseButton(QIcon(":/resources/icon_phase_combat_attackers.svg"));
	PhaseButton *combatBlockersButton = new PhaseButton(QIcon(":/resources/icon_phase_combat_blockers.svg"));
	PhaseButton *combatDamageButton = new PhaseButton(QIcon(":/resources/icon_phase_combat_damage.svg"));
	PhaseButton *combatEndButton = new PhaseButton(QIcon(":/resources/icon_phase_combat_end.svg"));
	PhaseButton *main2Button = new PhaseButton(QIcon(":/resources/icon_phase_main2.svg"));
	PhaseButton *cleanupButton = new PhaseButton(QIcon(":/resources/icon_phase_cleanup.svg"));
	
	buttonList << untapButton << upkeepButton << drawButton << main1Button << combatStartButton
		<< combatAttackersButton << combatBlockersButton << combatDamageButton << combatEndButton
		<< main2Button << cleanupButton;
	
	for (int i = 0; i < buttonList.size(); ++i)
		connect(buttonList[i], SIGNAL(clicked()), this, SLOT(phaseButtonClicked()));
	
	QPushButton *nextTurnButton = new QPushButton(QIcon(":/resources/icon_nextturn.svg"), QString());
	nextTurnButton->setIconSize(QSize(40, 40));
	nextTurnButton->setFixedSize(50, 50);
	connect(nextTurnButton, SIGNAL(clicked()), this, SIGNAL(signalNextTurn()));
		
	QVBoxLayout *layout = new QVBoxLayout;
	layout->setSpacing(0);
	
	layout->addStretch(1);
	layout->addWidget(untapButton);
	layout->addWidget(upkeepButton);
	layout->addWidget(drawButton);
	layout->addSpacing(10);
	layout->addWidget(main1Button);
	layout->addSpacing(10);
	layout->addWidget(combatStartButton);
	layout->addWidget(combatAttackersButton);
	layout->addWidget(combatBlockersButton);
	layout->addWidget(combatDamageButton);
	layout->addWidget(combatEndButton);
	layout->addSpacing(10);
	layout->addWidget(main2Button);
	layout->addSpacing(10);
	layout->addWidget(cleanupButton);
	layout->addSpacing(20);
	layout->addWidget(nextTurnButton);
	layout->addStretch(1);
	
	retranslateUi();
	setLayout(layout);
}

void PhasesToolbar::retranslateUi()
{
	buttonList[0]->setPhaseText(tr("Untap step"));
	buttonList[1]->setPhaseText(tr("Upkeep step"));
	buttonList[2]->setPhaseText(tr("Draw step"));
	buttonList[3]->setPhaseText(tr("First main phase"));
	buttonList[4]->setPhaseText(tr("Beginning of combat step"));
	buttonList[5]->setPhaseText(tr("Declare attackers step"));
	buttonList[6]->setPhaseText(tr("Declare blockers step"));
	buttonList[7]->setPhaseText(tr("Combat damage step"));
	buttonList[8]->setPhaseText(tr("End of combat step"));
	buttonList[9]->setPhaseText(tr("Second main phase"));
	buttonList[10]->setPhaseText(tr("End of turn step"));
}

void PhasesToolbar::setActivePhase(int phase)
{
	if (phase >= buttonList.size())
		return;
	
	for (int i = 0; i < buttonList.size(); ++i)
		buttonList[i]->setActive(i == phase);
}

void PhasesToolbar::phaseButtonClicked()
{
	PhaseButton *button = qobject_cast<PhaseButton *>(sender());
	if (button->getActive())
		return;
	emit signalSetPhase(buttonList.indexOf(button));
}
