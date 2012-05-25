#ifndef NEWSTABWIDGET_H
#define NEWSTABWIDGET_H

#include <QtGui>
#include <QtSql>
#include <QtWebKit>

#include "feedsmodel.h"
#include "feedsview.h"
#include "newsheader.h"
#include "newsmodel.h"
#include "newsview.h"
#include "webview.h"

class NewsTabWidget : public QWidget
{
  Q_OBJECT
private:
  void createNewsList();
  void createMenuNews();
  void createWebWidget();

  QWidget *newsWidget_;
  QMenu *newsContextMenu_;

  FeedsModel *feedsModel_;
  FeedsView *feedsView_;

  QAction *webHomePageAct_;
  QAction *webExternalBrowserAct_;

  QTimer *markNewsReadTimer_;

public:
  explicit NewsTabWidget(int feedId, QWidget *parent);
  void retranslateStrings();
  void setSettings();

  void updateWebView(QModelIndex index);

  int feedId_;

  NewsModel *newsModel_;
  NewsView *newsView_;
  NewsHeader *newsHeader_;
  QToolBar *newsToolBar_;

  QLabel *webPanelTitle_;
  QLabel *webPanelTitleLabel_;
  QLabel *webPanelAuthorLabel_;
  QLabel *webPanelAuthor_;
  QWidget *webPanel_;
  QWidget *webControlPanel_;

  QWidget *webWidget_;
  WebView *webView_;
  QProgressBar *webViewProgress_;
  QLabel *webViewProgressLabel_;

signals:
  void signalWebViewSetContent(QString content);

public slots:
  void slotNewsViewClicked(QModelIndex index);
  void slotNewsViewSelected(QModelIndex index, bool clicked = false);
  void slotNewsViewDoubleClicked(QModelIndex index);

private slots:
  void showContextMenuNews(const QPoint &p);
  void slotNewsUpPressed();
  void slotNewsDownPressed();
  void slotSetItemRead(QModelIndex index, int read);
  void slotSetItemStar(QModelIndex index, int starred);
  void slotReadTimer();

  void slotWebViewSetContent(QString content);
  void slotWebTitleLinkClicked(QString urlStr);
  void webHomePage();
  void openPageInExternalBrowser();
  void slotLinkClicked(QUrl url);
  void slotLinkHovered(const QString &link, const QString &, const QString &);
  void slotSetValue(int value);
  void slotLoadStarted();
  void slotLoadFinished(bool ok);
  void slotOpenNewsWebView();

};

#endif // NEWSTABWIDGET_H