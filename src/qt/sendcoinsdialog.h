// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SENDCOINSDIALOG_H
#define SENDCOINSDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QString>

class OptionsModel;
class SendCoinsEntry;
class SendCoinsRecipient;

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace Ui {
    class SendCoinsDialog;
}

struct CSendSentinel : public QObject
{
	Q_OBJECT

public slots:
	void userResponse( bool _accepted )
	{
		m_userResponse = _accepted;
		m_userResponded = true;
	}

signals:
	void requestAcceptance( uint _fee );

public:
	bool m_userResponded;
	bool m_userResponse;
};

extern CSendSentinel SendSentinel;

/** Dialog for sending bitcoins */
class SendCoinsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendCoinsDialog(QWidget *parent = 0);
    ~SendCoinsDialog();

    void setModel(WalletModel *model);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setAddress(const QString &address);
    void pasteEntry(const SendCoinsRecipient &rv);
    bool handlePaymentRequest(const SendCoinsRecipient &recipient);

public slots:
    void clear();
    void reject();
    void accept();
    SendCoinsEntry *addEntry();
    void updateTabsAndLabels();
    void setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance);
	void setTransactionStatus();
	void setAddressViewActive( bool _enable );

	void serviceTransactionUserAsk( uint _fee );
private:
    Ui::SendCoinsDialog *ui;
    WalletModel *model;
    bool fNewRecipientAllowed;
    AddressTableModel *m_addressModel;
    // Process WalletModel::SendCoinsReturn and generate a pair consisting
    // of a message and message flags for use in emit message().
    // Additional parameter msgArg can be used via .arg(msgArg).
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());

    bool getCoinAmount(QString & _amountToSend ) const;
    bool getWalletCoinAmount( QString & _walletCoinAmount ) const;
    bool fillCoinControl(qint64 const _outputSum,CCoinControl &_coinControl ) const;
private slots:
    void on_sendButton_clicked();
    void removeEntry(SendCoinsEntry* entry);
    void updateDisplayUnit();
    void coinControlFeatureChanged(bool);
    void coinControlButtonClicked();
    void coinControlChangeChecked(int);
    void coinControlChangeEdited(const QString &);
    void coinControlUpdateLabels();
    void coinControlClipboardQuantity();
    void coinControlClipboardAmount();
    void coinControlClipboardFee();
    void coinControlClipboardAfterFee();
    void coinControlClipboardBytes();
    void coinControlClipboardPriority();
    void coinControlClipboardLowOutput();
    void coinControlClipboardChange();

signals:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
	void userTransactionResonse( bool _response );
};

#endif // SENDCOINSDIALOG_H
