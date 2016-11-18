#include "ShareDialog.h"
#include "ui_ShareDialog.h"

#include <3rd_party/Helpers/Validators.h>

using UserInterface::ShareDialog;


ShareDialog::ShareDialog(QWidget *parent) :
	QLightBoxDialog(parent),
	m_ui(new Ui::ShareDialog)
{
	m_ui->setupUi(this);

	initView();
	initConnections();
}

ShareDialog::~ShareDialog()
{
	delete m_ui;
}

QString ShareDialog::email() const
{
	return m_ui->email->text();
}

int ShareDialog::role() const
{
	return m_ui->role->currentIndex();
}

void ShareDialog::initView()
{
	m_ui->buttons->addButton(tr("Share"), QDialogButtonBox::AcceptRole);

	QLightBoxDialog::initView();
}

void ShareDialog::initConnections()
{
	connect(m_ui->buttons, &QDialogButtonBox::accepted, [=] {
		if (Validator::isEmailValid(email())) {
			accept();
		} else {
			//
			// TODO: Показать ошибку о том, что плохой адрес электронной почты
			//
		}
	});
	connect(m_ui->buttons, &QDialogButtonBox::rejected, this, &ShareDialog::reject);

	QLightBoxDialog::initConnections();
}
