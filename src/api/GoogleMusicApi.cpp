/*
 * GoogleMusicApi.cpp
 *
 *  Created on: Nov 29, 2012
 *      Author: nmmrc18
 */

#include <QDebug>

#include "GoogleMusicApi.h"
#include "FormBuilder.h"
#include "GMusicDatasource.h"
#include "GoDataSource.h"
#include "AllAccessHelper.h"
#include <time.h>
#include "husha1.h"
#include "huctx.h"
#include "hugse56.h"
#include "hursa.h"
#include "huseed.h"
#include "hurandom.h"
#include "sbreturn.h"

#include <bb/data/JsonDataAccess>
#include <QtCore/QDir>
#include <QtCore/QLocale>

using namespace bb::data;

/*
 * Track Types:
 * public static final int TRACK_ORIGIN_NOT_USER_SELECTED = 4;
  public static final int TRACK_TYPE_NAUTILUS_EPHEMERAL = 7;
  public static final int TRACK_TYPE_NAUTILUS_PERSISTENT = 8;
  public static final int TRACK_TYPE_PROMO = 6;
  public static final int TRACK_TYPE_PURCHASED = 4;
 */

const QString GoogleMusicApi::MAIN_URL = QString("https://mclients.googleapis.com/sj/v2.4/");
const QString GoogleMusicApi::ACCOUNTS_LOGIN = QString("https://android.clients.google.com/auth");
const QString GoogleMusicApi::ISSUE_AUTH = QString("https://www.google.com/accounts/IssueAuthToken?service=gaia&Session=false&SID=%1&LSID=%2");
const QString GoogleMusicApi::TOKEN_AUTH = QString("https://www.google.com/accounts/TokenAuth?source=android-browser&auth=%1&continue=%2");
const QString GoogleMusicApi::PLAY_LISTEN = QString("https://play.google.com/music/listen?hl=en&u=0");
const QString GoogleMusicApi::HTTPS_LOADALLTRACKS = QString("%1trackfeed?dv=0&alt=json&include-tracks=true").arg(MAIN_URL);
const QString GoogleMusicApi::HTTPS_EPHEN_NUMERAL = QString("%1ephemeral/top?dv=0&alt=json&hl=en_US&updated-min=%2").arg(MAIN_URL);
const QString GoogleMusicApi::MUSIC_PLAY_SONGID = QString("https://play.google.com/music/play?u=0&songid=%1&pt=e");
// Note that the pt=e means explicit
//const QString GoogleMusicApi::MUSIC_PLAY_AASONGID = QString("https://play.google.com/music/play?u=0&slt=%1&songid=%2&sig=%3&pt=e");
const QString GoogleMusicApi::MUSIC_PLAY_AASONGID = QString("https://android.clients.google.com/music/mplay?slt=%1&songid=%2&sig=%3&pt=e&opt=hi&net=mob");
const QString GoogleMusicApi::MUSIC_PLAY_MJKSONG = QString("https://android.clients.google.com/music/mplay?slt=%1&mjck=%2&sig=%3&pt=e&opt=hi&net=mob");
//const QString GoogleMusicApi::MUSIC_PLAY_MJKSONG = QString("https://play.google.com/music/play?u=0&slt=%1&mjck=%2&sig=%3&pt=e");
const QString GoogleMusicApi::LOAD_PLAYLIST_TRACKS = QString("%1plentryfeed?dv=0&alt=json&updated-min=%2").arg(MAIN_URL);
const QString GoogleMusicApi::LOAD_PLAYLIST_FEED = QString("%1playlistfeed?dv=0&alt=json&updated-min=%2").arg(MAIN_URL);
//const QString GoogleMusicApi::UPDATE_META = QString("https://play.google.com/music/services/modifyentries");
const QString GoogleMusicApi::UPDATE_META = QString("%1trackbatch?dv=0&alt=json").arg(MAIN_URL);
const QString GoogleMusicApi::CREATE_RADIO = QString("https://play.google.com/music/services/radio/createstation");
const QString GoogleMusicApi::LOAD_RADIOS = QString("%1radio/station?dv=0&alt=json&updated-min=0").arg(MAIN_URL);
const QString GoogleMusicApi::LOAD_RADIO_STATION = QString("%1radio/stationfeed?dv=0&alt=json&updated-min=0").arg(MAIN_URL);
const QString GoogleMusicApi::ADD_AA_TO_LIBRARY = QString("%1trackbatch?dv=0&alt=json").arg(MAIN_URL);
//ct = ContentType: 1 = Song, 2 = Artist, 3 = Album, 4 = ?, 5 = ?
const QString GoogleMusicApi::SEARCH_ALL_ACCESS = QString("%1query?dv=0&q=%1&ct=1,2,3,4,5&max-results=10").arg(MAIN_URL);

const QString GoogleMusicApi::USER_CONFIG = QString("%1config?dv=0").arg(MAIN_URL);

const QString GoogleMusicApi::AUTHTOKEN = QString("authToken");
const QString GoogleMusicApi::IS_LOGGED_IN = QString("loggedIn");

const QString GoogleMusicApi::AUTH_KEY = QString("AAAAgMom/1a/v0lblO2Ubrt60J2gcuXSljGFQXgcyZWveWLEwo6prwgi3iJIZdodyhKZQrNWp5nKJ3srRXcUW+F1BD3baEVGcmEgqaLZUNBjm057pKRI16kB0YppeGx5qIQ5QjKzsR8ETQbKLNWgRY0QRNVz34kMJR3P/LgHax/6rmf5AAAAAwEAAQ==");

GoogleMusicApi::GoogleMusicApi()
: QObject(),
  saveLocation(0) {
}

GoogleMusicApi::GoogleMusicApi(QVariantMap &songToSync)
: QObject(),
  saveLocation(0)
{
	int size = songToSync.size();
	this->songToSync = new QVariantMap(songToSync);
	size = this->songToSync->size();
	size++;
}

void GoogleMusicApi::login() {
	connector = new UrlConnector;
	QLocale loc = QLocale().system();
	QVariantMap *fields = new QVariantMap;
	fields->insert("service", "ac2dm");
	fields->insert("has_permission", 1);
	fields->insert("add_account", 1);
	fields->insert("EncryptedPasswd", signature().replace("+", "-").replace("/", "_").replace("=","%3D"));
	fields->insert("accountType", "HOSTED_OR_GOOGLE");
	fields->insert("source", "android");
	fields->insert("sdk_version", 21);
	fields->insert("androidId", "24f1bb1fa9e2a3e9");
	fields->insert("device_country", "us");
	fields->insert("operatorCountry", "us");
	fields->insert("lang", "en");
	fields->insert("Email", QUrl::toPercentEncoding(*email));
	//fields->insert("Passwd", *password);

	qDebug() << "GMA: Logging in...";

	FormBuilder *form = new FormBuilder(false);
	form->addFieldsAsLine(fields);
	form->close();

	connect(connector, SIGNAL(complete(QString)), this, SLOT(loginAuthComplete(QString)), Qt::QueuedConnection);
	//connect(connector, SIGNAL(jsonDump(QString)), this, SLOT(setJsonDump(QString)));
	connect(this, SIGNAL(abortConnection()), connector, SLOT(cancelRequest()), Qt::UniqueConnection);
	connect(connector, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString, QString)));
	connector->post(ACCOUNTS_LOGIN, form);

}

QString GoogleMusicApi::signature() {
    QByteArray key = QByteArray::fromBase64(AUTH_KEY.toUtf8());
    // Now I need to read the first 4 bytes and turn it into a number?
    // this... is silly as it can be static...
    // I guess if the key changes...
    //QByteArray first4 = key.left(4);
    //int first4Int = 0x0 | (0xFF & first4.at(0)) << 24 | (0xFF & first4.at(1)) << 16 | (0xFF & first4.at(2)) << 8 | (0xFF & first4.at(3));

    // Lets just get the initial signature first
    sb_GlobalCtx globalContext;
    hu_GlobalCtxCreateDefault(&globalContext);
    hu_RegisterSbg56SHA1(globalContext);
    hu_RegisterSbg56RSA(globalContext);
    hu_RegisterSbg56(globalContext);
    hu_RegisterSystemSeed(globalContext);
    hu_InitSbg56(globalContext);
    sb_Context context = NULL;
    int success = hu_SHA1Begin(SB_SHA1_DIGEST_LEN, NULL, &context, globalContext);
    if (success != SB_SUCCESS) {
        return "";
    }
    unsigned char* keychar = reinterpret_cast<unsigned char*>(key.data());
    success = hu_SHA1Hash(context, (size_t)key.length(), keychar, globalContext);
    unsigned char hashedKey[SB_SHA1_DIGEST_LEN];
    success = hu_SHA1End(&context, hashedKey, globalContext);
    // now we just want the first 4 bytes of the has... again something that is static...
    QByteArray hashedKeyQB = QByteArray::fromRawData(reinterpret_cast<char*>(hashedKey), SB_SHA1_DIGEST_LEN);
    QByteArray signature = hashedKeyQB.left(4);

    QString loginCombined = QString("%1%2").arg(*email).arg(*password);
    unsigned char* emailChar = reinterpret_cast<unsigned char*>(email->toUtf8().data());
    unsigned char* passwordChar = reinterpret_cast<unsigned char*>(password->toUtf8().data());
    int emailCharSize = email->length();
    int passwordCharSize = password->length();
    unsigned char combinedChar[emailCharSize + passwordCharSize + 1];

    memcpy(combinedChar, emailChar, emailCharSize);
    char *null = {0x0};
    memcpy(&combinedChar[emailCharSize], &null, 1);
    memcpy(&combinedChar[emailCharSize+1], passwordChar, passwordCharSize);

    sb_PublicKey pubKey = NULL;

    unsigned char* loginChar = combinedChar;

    QByteArray exp = QByteArray::number(65537);
    unsigned char modChar[128] = { 0xCA, 0x26, 0xFF, 0x56, 0xBF, 0xBF,
            0x49, 0x5B, 0x94, 0xED, 0x94, 0x6E, 0xBB, 0x7A, 0xD0,
            0x9D, 0xA0, 0x72, 0xE5, 0xD2, 0x96, 0x31, 0x85, 0x41,
            0x78, 0x1C, 0xC9, 0x95, 0xAF, 0x79, 0x62, 0xC4, 0xC2,
            0x8E, 0xA9, 0xAF, 0x08, 0x22, 0xDE, 0x22, 0x48, 0x65,
            0xDA, 0x1D, 0xCA, 0x12, 0x99, 0x42, 0xB3, 0x56, 0xA7,
            0x99, 0xCA, 0x27, 0x7B, 0x2B, 0x45, 0x77, 0x14, 0x5B,
            0xE1, 0x75, 0x04, 0x3D, 0xDB, 0x68, 0x45, 0x46, 0x72,
            0x61, 0x20, 0xA9, 0xA2, 0xD9, 0x50, 0xD0, 0x63, 0x9B,
            0x4E, 0x7B, 0xA4, 0xA4, 0x48, 0xD7, 0xA9, 0x01, 0xD1,
            0x8A, 0x69, 0x78, 0x6C, 0x79, 0xA8, 0x84, 0x39, 0x42,
            0x32, 0xB3, 0xB1, 0x1F, 0x04, 0x4D, 0x06, 0xCA, 0x2C,
            0xD5, 0xA0, 0x45, 0x8D, 0x10, 0x44, 0xD5, 0x73, 0xDF,
            0x89, 0x0C, 0x25, 0x1D, 0xCF, 0xFC, 0xB8, 0x07, 0x6B,
            0x1F, 0xFA, 0xAE, 0x67, 0xF9
    };
    //{-54, 38, -1, 86, -65, -65, 73, 91, -108, -19, -108, 110, -69, 122, -48, -99, -96, 114, -27, -46, -106, 49, -123, 65, 120, 28, -55, -107, -81, 121, 98, -60, -62, -114, -87, -81, 8, 34, -34, 34, 72, 101, -38, 29, -54, 18, -103, 66, -77, 86, -89, -103, -54, 39, 123, 43, 69, 119, 20, 91, -31, 117, 4, 61, -37, 104, 69, 70, 114, 97, 32, -87, -94, -39, 80, -48, 99, -101, 78, 123, -92, -92, 72, -41, -87, 1, -47, -118, 105, 120, 108, 121, -88, -124, 57, 66, 50, -77, -79, 31, 4, 77, 6, -54, 44, -43, -96, 69, -115, 16, 68, -43, 115, -33, -119, 12, 37, 29, -49, -4, -72, 7, 107, 31, -6, -82, 103, -7};
    QByteArray mod = QByteArray::fromRawData(reinterpret_cast<char*>(modChar), (size_t)128);
    //qulonglong modInt = 141956196257934770187925561804359820971448272350983018436093173897855484510782207920697285059648243152878542520514658971720228524276304322321325896163977435852395272134149378260200371457183474602754725451457370420041505749329659663863538423736961928495802209949126722610439862310060378247113201580053877385209;
    //QByteArray mod = QByteArray::number(modInt);
    //unsigned char *expChar = reinterpret_cast<unsigned char*>(exp.data());
    //unsigned char *modChar = reinterpret_cast<unsigned char*>(mod.data());
    uchar mySeed[4];
    size_t seedLen = 4;

    success = hu_SeedGet(&seedLen, mySeed, globalContext);
    sb_RNGCtx rngCtx = NULL;
    success = hu_RngCreate(seedLen, mySeed, NULL, NULL, NULL, &rngCtx, globalContext);
    sb_Params rsaParams = 0;
    success = hu_RSAParamsCreate(size_t(1024), rngCtx, NULL, &(rsaParams), globalContext);
    success = hu_RSAKeySet(rsaParams, 0, NULL, (size_t)128, modChar, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, 0, NULL, NULL, &pubKey, globalContext);
    //Dsuccess = hu_RSAKeyGet(rsaParams, NULL, pubKey, (size_t*)exp.length(), expChar, (size_t*)mod.length(), modChar, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, globalContext);

    // Hopefully we have a pub key now
    unsigned char output[mod.length()];
    int sizeOfShit = mod.length();
    //success = hu_RSAPKCS1v15Enc(rsaParams, pubKey, size_t(loginCombined.length()), loginChar, (size_t*)sizeOfShit, output, globalContext);
    int sizeOfOtherShit = loginCombined.length()+1;
    success = hu_RSAPKCS1v21SHA1Encrypt(rsaParams, pubKey, (size_t)0, NULL, size_t(sizeOfOtherShit), loginChar, (size_t*)&sizeOfShit, output, globalContext);
    //success = hu_RSAPublicEncrypt(rsaParams, pubKey, loginChar, output, globalContext);

    QByteArray signatureOutput = QByteArray(0);
    char zero[1] = { 0 };
    signatureOutput.append(zero, 1);
    signatureOutput.append(signature);
    QByteArray loginEncrypted = QByteArray::fromRawData(reinterpret_cast<char*>(output), 128);
    signatureOutput.append(loginEncrypted);


    // Now we need to do a PUBLIC_KEY encryption of the loginCombined
    /*
     * Rules:
     * Signature needs to have this format:
     * signature[0] = 0
     * signature[1 - 4] = first 4 bytes of SHA1
     * signature[5 - 132] = encrypted login+password
     */

    QString signatureString = QString(signatureOutput.toBase64());
    return signatureString;
}

void GoogleMusicApi::loginAuthComplete(QString token) {
    disconnect(connector, SIGNAL(complete(QString)), this, SLOT(loginAuthComplete(QString)));
    QVariantMap *fields = new QVariantMap;
    fields->insert("accountType", "HOSTED_OR_GOOGLE");
    fields->insert("Email", QUrl::toPercentEncoding(*email));
    fields->insert("has_permission", 1);
    fields->insert("EncryptedPasswd", token);
    fields->insert("service", "sj");
    fields->insert("source", "android");
    fields->insert("androidId", "24f1bb1fa9e2a3e9");
    fields->insert("app", "GoMusic");
    fields->insert("client_sig", "38918a453d07199354f8b19af05ec6562ced5788");
    fields->insert("device_country", "us");
    fields->insert("operatorCountry", "us");
    fields->insert("lang", "en");
    fields->insert("sdk_version", 21);

    FormBuilder *form = new FormBuilder(false);
    form->addFieldsAsLine(fields);
    form->close();

    connect(connector, SIGNAL(complete()), this, SLOT(loginComplete()), Qt::QueuedConnection);
    //connect(connector, SIGNAL(jsonDump(QString)), this, SLOT(setJsonDump(QString)));
    connect(this, SIGNAL(abortConnection()), connector, SLOT(cancelRequest()), Qt::UniqueConnection);
    connect(connector, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString, QString)));
    connector->post(ACCOUNTS_LOGIN, form);
}

void GoogleMusicApi::loginComplete() {
	disconnect(connector, SIGNAL(complete()), this, SLOT(loginComplete()));
	emit complete();
}

void GoogleMusicApi::updateUserConfig() {
	connector = new UrlConnector;

	connect(connector, SIGNAL(complete(QString)), this, SLOT(onUserConfigUpdate(QString)), Qt::QueuedConnection);
	connector->get(USER_CONFIG);
}

void GoogleMusicApi::onUserConfigUpdate(QString response) {
	JsonDataAccess jda;
	QVariant jsonVar = jda.loadFromBuffer(response);
	QVariant dataVar = jsonVar.toMap().value("data");
	QVariant entriesVar = dataVar.toMap().value("entries");

	Q_FOREACH(QVariant var, entriesVar.toList()) {
		QVariantMap entry = var.toMap();
		if (entry.value("key").toString() == "isNautilusUser") {
			bool isNaut = entry.value("value").toBool();
			QSettings settings;
			settings.setValue("Nautilus", isNaut);
		}
	}

	emit complete();
}

void GoogleMusicApi::setJsonDump(QString dump) {
	emit jsonDump(dump);
}

void GoogleMusicApi::getAllSongs() {
	getSongs(QString(""));
}

void GoogleMusicApi::getSongs(QString continuationToken) {
	QVariant songs;

	QVariantMap *fields = new QVariantMap;
	if (continuationToken.isEmpty()) {
		fields->insert("", "{\"max-results\":1000,\"start-token\":\"0\"}");
	} else {
		fields->insert("", QString("{\"max-results\":1000,\"start-token\":\"%1\"}").arg(continuationToken));
	}
	//fields->insert("json", "{ \"start-token\" : \"\" }");
	//fields->insert("json", QString("{\"continuationToken\":\"%1\"}").arg(continuationToken));

	FormBuilder *form = new FormBuilder(false);
	form->addFieldsAsLine(fields);
	form->close();

	connector = new UrlConnector;
	// this doesn't make any sense but whatever

	connect(connector, SIGNAL(complete(QString)), this, SLOT(requestComplete(QString)));
	connect(connector, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString, QString)));
	connector->post(QString(HTTPS_LOADALLTRACKS), form);
}

void GoogleMusicApi::getRecentSongs() {
	QVariantMap *fields = new QVariantMap;
	fields->insert("", "{\"max-results\" : 1000, \"start-token\" : \"0\"}");

	FormBuilder *form = new FormBuilder(false);
	form->addFieldsAsLine(fields);
	form->close();

	connector = new UrlConnector;
		// this doesn't make any sense but whatever

	connect(connector, SIGNAL(complete(QString)), this, SLOT(requestComplete(QString)));
	connect(connector, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString, QString)));

	// select * from song order by lastModifiedTimestamp desc limit 1
	GoDataSource *data = new GoDataSource();
	data->setConnectionName("timestamp");
	QVariantList results = data->runQuery("select * from song order by lastModifiedTimestamp desc limit 1", false);
	QVariantMap mostRecentSong = results.value(0).toMap();
	qint64 minTime = mostRecentSong.value("lastModifiedTimestamp").toLongLong();

	data->deleteLater();

	//QDateTime today = QDateTime().currentDateTime();
	//qint64 time = today.toMSecsSinceEpoch() * 1000;
	//qint64 minTime = time - 604800000000;
	// the real secret is the last modified date for all songs
	QString url = QString(HTTPS_LOADALLTRACKS).append("&updated-min=%1").arg(minTime);

	connector->post(url, form);
}

void GoogleMusicApi::getEphenNumeral() {
	QVariantMap *fields = new QVariantMap;
	fields->insert("", "{\"max-results\":1000}");

	FormBuilder *form = new FormBuilder(false);
	form->addFieldsAsLine(fields);
	form->close();

	GoDataSource *data = new GoDataSource();
	data->setConnectionName("timestamp");
	QVariantList results = data->runQuery("select * from song order by lastModifiedTimestamp desc limit 1", false);
	QVariantMap mostRecentSong = results.value(0).toMap();
	qint64 minTime = mostRecentSong.value("lastModifiedTimestamp").toLongLong();

	data->deleteLater();

	connector = new UrlConnector;
	connect(connector, SIGNAL(complete(QString)), this, SLOT(requestComplete(QString)));
	connect(connector, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString, QString)));

	connector->post(QString(HTTPS_EPHEN_NUMERAL).arg(minTime), form);
}

void GoogleMusicApi::loadRadios() {
	connector = new UrlConnector;

	connect(connector, SIGNAL(complete(QString)), this, SLOT(allAccessCheckComplete(QString)));
	connect(connector, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString, QString)));

	connector->get(QString(SEARCH_ALL_ACCESS).arg("cady"));
	//connector->post(QString(LOAD_RADIOS), form);
}

void GoogleMusicApi::allAccessCheckComplete(QString response) {
	QVariantMap *fields = new QVariantMap;
	fields->insert("", "{\"max-results\" : \"1000\" }");

	FormBuilder *form = new FormBuilder(false);
	form->addFieldsAsLine(fields);
	form->close();

	connector = new UrlConnector;

	connect(connector, SIGNAL(complete(QString)), this, SLOT(radioStationsComplete(QString)));
	connect(connector, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString, QString)));

	connector->post(QString(LOAD_RADIOS), form);
}

void GoogleMusicApi::radioStationsComplete(QString list) {
	/*
	 * {
 "kind": "sj#radioList",
 "data": {
  "items": [
   {
    "kind": "sj#radioStation",
    "id": "ca9cd622-cfbf-3326-abb9-6491251fc40b",
    "clientId": "B4lwcudnanpwu242e3lenkijksm",
    "lastModifiedTimestamp": "1368736186804101",
    "recentTimestamp": "1368736185864000",
    "name": "Siberia",
    "seed": {
     "kind": "sj#radioSeed",
     "albumId": "B4lwcudnanpwu242e3lenkijksm"
    },

    so we will have
    object -> data
    			-> items
    				-> [ radios ]
	 */
	JsonDataAccess jda;
	QVariant jsonVar = jda.loadFromBuffer(list);
	QVariant dataVar = jsonVar.toMap().value("data");
	QVariant itemsVar = dataVar.toMap().value("items");
	QVariantList listOfRadios = itemsVar.toList();
	QVariantList listToReturn = QVariantList();
	Q_FOREACH(QVariant radio, listOfRadios) {
		if (!radio.toMap().value("deleted").toBool()) {
			listToReturn.append(radio);
		}
	}

	emit radioStations(listToReturn);
}

void GoogleMusicApi::loadSong() {
	loadSong(songToDownload);
}

void GoogleMusicApi::loadSong(const QVariantMap song) {
	if (!song.value("id").isNull()) {
		songIdToSave = song.value("id").toString();
	} else if (!song.value("nid").isNull()) {
		songIdToSave = song.value("nid").toString();
	}
	QString url;
	int trackType = -1;
	if (!song.value("trackType").isNull()) {
		trackType = song.value("trackType").toInt();
	} else {
		trackType = song.value("type").toInt();
	}

	qDebug() << "GMA: TrackType: " << trackType;

	/*
	 * Well this is helpful
	  arrayOfType[0] = RADIO;
      arrayOfType[1] = TRACK_SEED;
      arrayOfType[2] = ALBUM_SEED;
      arrayOfType[3] = ARTIST_SEED;
      arrayOfType[4] = GENRE_SEED;
	 */

	//if (songIdToSave.startsWith("T")) {
		// All Access Type
		AllAccessHelper * helper = new AllAccessHelper;
		QString salt = helper->getSalt();
		QString saltAndSong(songIdToSave + salt);
		unsigned char message[SB_HMAC_SHA1_160_TAG_LEN];
		helper->getSig(saltAndSong, message);
		QByteArray mess = QByteArray::fromRawData(reinterpret_cast<char*>(message), SB_HMAC_SHA1_160_TAG_LEN);
		QString hash = QString::fromUtf8(mess.toBase64());
		hash = hash.replace("+", "-");
		hash = hash.replace("/", "_");
		QString sig = hash.replace("=", "");
		if (!songIdToSave.startsWith("T")) {
			//url = QString("https://android.clients.google.com/music/mplay?songid=%1&opt=hi&dt=pc").arg(songIdToSave);
			url = QString(MUSIC_PLAY_AASONGID.arg(salt).arg(songIdToSave).arg(sig));
		} else {
			url = QString(MUSIC_PLAY_MJKSONG.arg(salt).arg(songIdToSave).arg(sig));
		}
		helper->deleteLater();
	//} else {
		//url = MUSIC_PLAY_SONGID.arg(songIdToSave);
	//}
	qDebug() << "GMAL URL: " << url;
	connector = NULL;
	connector = new UrlConnector;
	connect(connector, SIGNAL(complete(QString)), this, SLOT(onSongUrl(QString)), Qt::UniqueConnection);
	connect(this, SIGNAL(abortConnection()), connector, SLOT(cancelRequest()), Qt::UniqueConnection);
	connect(connector, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString,QString)));
	connector->get(url);
}

void GoogleMusicApi::onSongUrl(QString response) {
	disconnect(connector, SIGNAL(complete(QString)), this, SLOT(onSongUrl(QString)));

	JsonDataAccess jda;
	QVariant songUrlVar = jda.loadFromBuffer(response);
	QVariantMap songUrl = songUrlVar.toMap();
	GoDataSource *data = new GoDataSource();
	data->setConnectionName("api");
	QVariant songVar = data->getSong(songIdToSave);
	QVariantMap song = songVar.toMap();
	saveLocation = song.value("shouldBeLocal").toInt();
	if (!songUrl["url"].isNull()) {
		// its an uploaded song

		QString fileLocation;

		if (saveLocation == 1) {
			QString title(song.value("title").toString());
			QString artist(song.value("artist").toString());
			QString album(song.value("album").toString());
			QString songWithBaddys = QString("%1 - %2 - %3").arg(title).arg(artist).arg(album);
			songNameToSave = fixLocalName(songWithBaddys);
			if (saveLocationName.contains("sdcard")) {
				// save to SD Card
				QDir sdcard = QDir(QDir::currentPath() + "/../../removable/sdcard");
				if (sdcard.exists()) {
					// we can save to SD Card because it exists
					fileLocation = QString(QDir::currentPath() + "/../../removable/sdcard/music/%1.mp3").arg(songNameToSave);
					QString baddyLocation = QString(QDir::currentPath() + "/../../removable/sdcard/music/%1.mp3").arg(songWithBaddys);
					QFile baddyFile(baddyLocation);
					if (baddyFile.exists()) {
						baddyFile.rename(songNameToSave);
					}
				} else {
					// save to device
					fileLocation = QString(QDir::currentPath()+"/shared/music/%1.mp3").arg(songNameToSave);
					QString baddyLocation = QString(QDir::currentPath()+"/shared/music/%1.mp3").arg(songWithBaddys);
					QFile baddyFile(baddyLocation);
					if (baddyFile.exists()) {
						baddyFile.rename(songNameToSave);
					}
				}
			} else {
				// save to device
				fileLocation = QString(QDir::currentPath()+"/shared/music/%1.mp3").arg(songNameToSave);
				QString baddyLocation = QString(QDir::currentPath()+"/shared/music/%1.mp3").arg(songWithBaddys);
				QFile baddyFile(baddyLocation);
				if (baddyFile.exists()) {
					baddyFile.rename(songNameToSave);
				}
			}
		} else {
			fileLocation = QString(QDir::homePath() + "/temp/%1.mp3").arg(songIdToSave);
		}
		emit urlLocation(fileLocation);

		UrlConnector *newCon = new UrlConnector;
		newCon->fileToWriteTo = new QFile(fileLocation);
		qDebug() << "GMA: Saving as: " << fileLocation;
		newCon->bytes = true;

		connect(newCon, SIGNAL(downloadProgress(qint64,qint64)), this,
							SLOT(updateProgress(qint64,qint64)));
		connect(newCon, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString,QString)));
		connect(this, SIGNAL(abortConnection()), newCon, SLOT(cancelRequest()), Qt::UniqueConnection);
		connect(newCon, SIGNAL(complete(QByteArray)), this, SLOT(onSaveSong(QByteArray)));

		newCon->get(songUrl["url"].toString());
		connector = NULL;
		connector = newCon;
	} else if (!songUrl["urls"].isNull()) {
		// its a streaming All Access deal
		emit allAccess(true);
		QString fileLocation;
		if (saveLocation == 1) {
			/*QSettings settings;
			bool val = settings.value("showedWarningAboutAllAccess", false).toBool();
			GoDataSource *data = new GoDataSource();
			data->setConnectionName("apiz");
			QString id(song.value("id").toString());
			QString query = QString("update song set shouldBeLocal = 0 where id = '%1'").arg(id);
			data->setQuery(query);
			data->runQuery();
			data->deleteLater();

			if (!val) {
				settings.setValue("showedWarningAboutAllAccess", true);
				emit error("All Access Song", "You cannot save All Access music to your device.");
				emit complete();
			}
			*/
			fileLocation = QString(QDir::homePath() + "/allaccess/%1.mp3").arg(songIdToSave);
		} else {

			fileLocation = QString(QDir::homePath() + "/temp/%1.mp3").arg(songIdToSave);

		}
		connector = NULL;
		connector = new UrlConnector;
		connector->fileToWriteTo = new QFile(fileLocation);
		if (connector->fileToWriteTo->exists()) {

		}
		connector->bytes = true;

		urlsToDownload = songUrl["urls"].toList();
		currIndexInUrls = 0;
				//connect(connector, SIGNAL(downloadProgress(qint64,qint64)), this,
					//									SLOT(updateProgress(qint64,qint64)));

		connect(connector, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString,QString)));
		connect(this, SIGNAL(abortConnection()), connector, SLOT(cancelRequest()), Qt::UniqueConnection);
		connect(connector, SIGNAL(complete(QByteArray)), this, SLOT(onAllAccessUrl(QByteArray)));

		qDebug() << "GMA: SongID: " << songIdToSave;

		onAllAccessUrl(NULL);
		//emit error("All Access Error", "Unable to play songs added via All Access, but I'm working on it :)");
	}
}

void GoogleMusicApi::onAllAccessUrl(QByteArray unused) {
	Q_UNUSED(unused);

	if (currIndexInUrls != urlsToDownload.size()) {
		QString url = urlsToDownload.value(currIndexInUrls).toString();
		QString urlToUse = urlsToDownload.value(currIndexInUrls).toString();
		emit downloadProgress(currIndexInUrls, urlsToDownload.size());
		currIndexInUrls++;
		QRegExp range = QRegExp(".*range=");
		QString z = url.remove(range);
		//qDebug() << "GMA: z = " << z;
		QRegExp segment = QRegExp("&segment=.*");
		QString x = z.remove(segment);
		//qDebug() << "GMA: x = " << x;
		QStringList values = x.split("-", QString::SkipEmptyParts);

		connector->startingByte = values.at(0).toInt();
		connector->endingByte = values.at(1).toInt();
		//qDebug() << "GMA: From byte: " << values.at(0).toInt() << " to byte: " << values.at(1).toInt();
		//qDebug() << "GMA: Url " << currIndexInUrls << " of " << urlsToDownload.size();
		//qDebug() << "GMA: Getting url " << urlToUse;
		if (urlToUse.isEmpty()) {
			connector->fileToWriteTo->close();
			emit downloadProgress(urlsToDownload.size(), urlsToDownload.size());
			emit complete();
		} else {
			connector->get(urlToUse);
		}
	} else {
		connector->fileToWriteTo->close();
		emit downloadProgress(urlsToDownload.size(), urlsToDownload.size());
		emit complete();
	}
}

void GoogleMusicApi::onSaveSong(QByteArray songData) {
	disconnect(connector, SIGNAL(complete(QByteArray)), this, SLOT(onSaveSong(QByteArray)));
	disconnect(connector, SIGNAL(downloadProgress(qint64,qint64)), this,
						SLOT(updateProgress(qint64,qint64)));
	Q_UNUSED(songData);
	emit complete();
}

void GoogleMusicApi::getAllPlaylists() {
	QVariantMap *fields = new QVariantMap;
	fields->insert("", "{\"max-results\" : \"1000\" }");

	FormBuilder *form = new FormBuilder(false);
	form->addFieldsAsLine(fields);
	form->close();

	GoDataSource *data = new GoDataSource();
	data->setConnectionName("timestamp");
	QVariantList results = data->runQuery("select * from playlist order by lastModifiedTimestamp desc limit 1", false);
	qint64 minTime = 0;
	if (results.size() != 0) {
		QVariantMap mostRecentSong = results.value(0).toMap();
		minTime = mostRecentSong.value("lastModifiedTimestamp").toLongLong();
	}
	data->deleteLater();

	connector = new UrlConnector;

	connect(connector, SIGNAL(complete(QString)), this, SLOT(requestComplete(QString)));
	connect(connector, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString,QString)));
	connector->post(QString(LOAD_PLAYLIST_FEED).arg(minTime), form);
}

void GoogleMusicApi::getAllPlaylistSongs() {
	QVariantMap *fields = new QVariantMap;
	fields->insert("", "{\"max-results\" : \"1000\" }");

	FormBuilder *form = new FormBuilder(false);
	form->addFieldsAsLine(fields);
	form->close();

	GoDataSource *data = new GoDataSource();
	data->setConnectionName("timestamp");
	QVariantList results = data->runQuery("select * from playlistSongs order by lastModifiedTimestamp desc limit 1", false);
	qint64 minTime = 0;
	if (results.size() != 0) {
		QVariantMap mostRecentSong = results.value(0).toMap();
		minTime = mostRecentSong.value("lastModifiedTimestamp").toLongLong();
	}
	data->deleteLater();

	connector = new UrlConnector;

	connect(connector, SIGNAL(complete(QString)), this, SLOT(requestComplete(QString)));
	connect(connector, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString,QString)));

	connector->post(QString(LOAD_PLAYLIST_TRACKS).arg(minTime), form);
}

void GoogleMusicApi::getPlaylist(QString id) {
	/*QString pid;
	if (id == NULL) {
		// get All
		pid = QString("{}");
		fields->insert("json", pid);
	} else {
		pid = QString(id);
		fields->insert("json", QString("{\"id\": \"%1\"}").arg(pid));
	}


	*/
	QVariantMap *fields = new QVariantMap;
	if (id == NULL) {
		fields->insert("", "{\"max-results\" : \"1000\" }");
	} else {
		fields->insert("", QString("{\"max-results\" : \"1000\", \"start-token\" : \"%1\"}").arg(id));
	}

	FormBuilder *form = new FormBuilder(false);
	form->addFields(fields);
	form->close();

	connector = new UrlConnector;

	connect(connector, SIGNAL(complete(QString)), this, SLOT(requestComplete(QString)));
	connect(connector, SIGNAL(networkError(QString,QString)), this, SLOT(onError(QString,QString)));
	connector->post(QString(LOAD_PLAYLIST_FEED), form);
}

void GoogleMusicApi::getRadio(QString songId, QVariant seed, QList<QVariantMap> recentlyPlayed) {
	QVariantMap *fields = new QVariantMap;
	QString seedId;
	QVariantMap seedMap = seed.toMap();
	if (!seedMap.value("albumId").isNull()) {
		seedId = "albumId";
	} else if (!seedMap.value("genreId").isNull()) {
		seedId = "genreId";
	} else if (!seedMap.value("artistId").isNull()) {
		seedId = "artistId";
	} else if (!seedMap.value("trackId").isNull()) {
		seedId = "trackId";
	} else if (!seedMap.value("trackLockerId").isNull()) {
		seedId = "trackLockerId";
	}

	QString seedKey = seedMap.value(seedId).toString();

	QString recentlyPlayedJson = "";
	if (!recentlyPlayed.isEmpty()) {
		int j = 0;
		for (int i = recentlyPlayed.length(); j < 5; i--) {
			QVariantMap song = recentlyPlayed.at(i-1);
			QString recentJson = QString("{ \"id\" : \"%1\", \"type\" : \"%2\" }").arg(song.value("nid").toString()).arg(song.value("trackType").toString());
			j++;
			if (j != 5) {
				recentlyPlayedJson.append(recentJson + ",");
			} else {
				recentlyPlayedJson.append(recentJson);
			}
		}
	}

	fields->insert("",
			QString("{ \"stations\" : [{\"radioId\" : \"%1\",\"numEntries\" : \"25\", \"recentlyPlayed\" : [%2], \"contentFilter\" : 1, \"seed\" : { \"%3\" : \"%4\"}}]}")
			.arg(songId)
			.arg(recentlyPlayedJson)
			.arg(seedId)
			.arg(seedKey));

	FormBuilder *form = new FormBuilder(false);
	form->addFields(fields);
	form->close();

	connector = new UrlConnector;
	connect(connector, SIGNAL(complete(QString)), this, SLOT(radioPlaylistComplete(QString)));
	connector->post(QString(LOAD_RADIO_STATION), form);
}

void GoogleMusicApi::radioPlaylistComplete(QString response) {
	JsonDataAccess jda;
	QVariant jsonVar = jda.loadFromBuffer(response);
	QVariant jsonData = jsonVar.toMap().value("data");
	QVariant jsonItems = jsonData.toMap().value("items");
	QVariant jsonTracks = jsonItems.toList().at(0).toMap().value("tracks");

	emit radioPlaylist(jsonTracks.toList());
}

void GoogleMusicApi::syncMetaData(QVariantMap &songToSync) {
	QVariantMap *fields = new QVariantMap;

	// ****** WARNING WARNING THE BELOW LOGIC IS DESIGNED ONLY TO SYNC THUMBS UP AND DOWN!!! *******
	QString songData = QString("{\"mutations\":[{\"update\":{\"deleted\":false, \"id\":");
	QString id = songToSync.value("id").toString();
	songData = songData + QString("\"%1\",").arg(id);
	unsigned int t = time(NULL);
	int timeInMillis = t*1000;
	songData = songData + QString("\"lastModifiedTimestamp\":\"%d\",").arg(timeInMillis);
	int playCount = songToSync.value("playCount").toInt();
	int rating = songToSync.value("rating").toInt();
	songData = songData + QString("\"playCount\":%d, \"rating\":\"%d\",").arg(playCount).arg(rating);
	int type = -1;
	if (songToSync.value("trackType").isNull()) {
		type = songToSync.value("trackType").toInt();
	} else {
		type = songToSync.value("type").toInt();
	}
	songData = songData + QString("\"trackType\":%d").arg(type);
	songData = songData + QString("}}]}");

	/*
	 * {"mutations":[{"update":{"beatsPerMinute":-1,"creationTimestamp":"-1","deleted":false,"discNumber":-1,"durationMillis":"-1",
	 * "estimatedSize":"-1","id":"87b13ae1-1279-3416-97af-388551805bcc","lastModifiedTimestamp":"1379446688307974",
	 * "playCount":0,"rating":"5","totalDiscCount":-1,"trackNumber":-1,"trackType":8,"year":-1}}]}
	 */
	//GoDataSource *ds = new GoDataSource();
	//this->songToSync = &songToSync;



	/*Q_FOREACH(QVariant keyy, GoDataSource::SONG_KEYS) {
		QString key = QString(keyy.toString());
		if (!key.contains("shouldBeLocal") && !key.contains("downloadCompleted") &&
				!key.contains("metaSynced") && !key.contains("track") && (key.contains("id") || key.contains("rating"))) {
			songData = songData + QString("\"%1\":").arg(key);
			QVariant value = songToSync.value(key);
			QString valueStr = value.toString();
			if (value.type() == QVariant::String) {
				songData = songData + QString("\"%1\"").arg(valueStr);
			} else if (value.type() == QVariant::LongLong || value.type() == QVariant::Int ||
				value.type() == QVariant::ULongLong) {
				if (!key.contains("deleted") && !key.contains("subjectToCuration")) {
					songData = songData + QString("%1").arg(valueStr);
				} else {
					int intVal = value.toInt();
					if (intVal == 1) {
						songData = songData + QString("true");
					} else {
						songData = songData + QString("false");
					}
				}
			}
			if (!key.contains("rating")) {
				songData = songData + QString(",");
			}
		}

	}
	QString json = QString("{\"entries\":[{%1}]}").arg(songData);*/
	/*QVariant id = songToSync.value("id");
	//if (!id.isNull()) {
		QString idS = id.toString();
		fields->insert("", QString("{\"create\" : { \"nid\" : \"%1\", \"rating\" : \"5\", \"trackType\" : 8 }}").arg(idS));
	//}
	*/

	fields->insert("", songData);
	/*
	 * Required:
	 * QStrings - composer, album, albumArtist, genre, name, artist, comment, id,
	 * 		matchedId, title, titleNorm
	 * QInt - durationInMillis, creationDate, type, beatsPerMin, recentTimestamp, rating, playCount
	 * bool - deleted, subjectToCuration
	 */

	FormBuilder *form = new FormBuilder(false);
	form->addFields(fields);
	form->close();
	connector = new UrlConnector;

	connect(connector, SIGNAL(complete(QString)), this, SLOT(basicComplete(QString)));
	connector->post(QString(UPDATE_META), form);
}

void GoogleMusicApi::startPlaylistDownload() {

	size = playlistToDownload.size();
	currPos = 0;
	emit updatePlaylistCount(currPos, size);
	connect(this, SIGNAL(complete()), this, SLOT(moveToMusicDir()), Qt::QueuedConnection);

	downloadPlaylistSong();
}

void GoogleMusicApi::downloadPlaylistSong() {
	QVariantMap songMap = playlistToDownload.at(currPos).toMap();
	QString title(songMap.value("title").toString());
	QString artist(songMap.value("artist").toString());
	QString album(songMap.value("album").toString());
	QString songName = QString("%1 - %2 - %3").arg(title).arg(artist).arg(album);
	songName = fixLocalName(songName);
	QString fileLocation;
	if (saveLocationName.contains("sdcard")) {
		// save to SD Card
		QDir sdcard = QDir(QDir::currentPath() + "/../../removable/sdcard");
		if (sdcard.exists()) {
			// we can save to SD Card because it exists
			fileLocation = QString(QDir::currentPath() + "/../../removable/sdcard/music/%1.mp3").arg(songName);
		} else {
			// save to device
			fileLocation = QString(QDir::currentPath()+"/shared/music/%1.mp3").arg(songName);
		}
	} else {
		// save to device
		fileLocation = QString(QDir::currentPath()+"/shared/music/%1.mp3").arg(songName);
	}

	QFile file(fileLocation);
	if (file.exists()) {

		QString id(songMap.value("id").toString());
		if (!songMap.value("downloadCompleted").toBool()) {
			// at this level, I will assume its not downloaded, but there
			// will be a secondary check at URL connector
			GoDataSource *data = new GoDataSource();
			data->setConnectionName("apii");
			QString query = QString("update song set shouldBeLocal = 1 where id = '%1'").arg(id);
			data->setQuery(query);
			data->runQuery();
			loadSong(songMap);
		} else {
			moveToMusicDir();
		}
	} else {
		GoDataSource *data = new GoDataSource();
		data->setConnectionName("apii");
		QString id(songMap.value("id").toString());
		QString query = QString("update song set shouldBeLocal = 1 where id = '%1'").arg(id);
		data->setQuery(query);
		data->runQuery();
		loadSong(songMap);
	}
}

void GoogleMusicApi::moveToMusicDir() {
	currPos++;
	emit updatePlaylistCount(currPos, size);
	//emit updatePlaylistProgress(currPos, size);
	if (currPos != size) {
		downloadPlaylistSong();
	} else {
		disconnect(this, SIGNAL(complete()), this, SLOT(moveToMusicDir()));
		emit playlistDownloadComplete();
	}
}

void GoogleMusicApi::renewCookie() {
	QSettings settings;
	settings.remove("cookies");
}

void GoogleMusicApi::setAlbumArtUrl(QString url) {
	albumArtUrl = new QString(url);
}

void GoogleMusicApi::getAlbumArt() {
	connector = new UrlConnector;
	connector->bytes = true;
	connector->fileToWriteTo = toPassToUrlCon;
	if (albumArtUrl->contains("s130"))
		connector->cacheNum = 1;
	else
		connector->cacheNum = 2;
	connect(connector, SIGNAL(complete(QByteArray)), this, SLOT(albumArtDataComplete(QByteArray)), Qt::UniqueConnection);
	connect(this, SIGNAL(abortConnection()), connector, SLOT(cancelRequest()), Qt::UniqueConnection);
	connect(this, SIGNAL(complete()), connector, SLOT(deleteLater()), Qt::UniqueConnection);
	connect(connector, SIGNAL(failed()), this, SLOT(onFailed()));

	connector->get(*albumArtUrl);
}

void GoogleMusicApi::onFailed() {
	disconnect(this, SIGNAL(abortConnection()), connector, SLOT(cancelRequest()));
	qDebug() << "GMA: Failed to connect";
	emit failed();
}

void GoogleMusicApi::onError(QString title, QString message) {
	emit error(title, message);
	emit failed();
}

void GoogleMusicApi::updateProgress(qint64 currSize, qint64 actSize) {
	emit downloadProgress(currSize, actSize);
}

void GoogleMusicApi::requestComplete(QString response) {
	disconnect(connector, SIGNAL(complete(QString)), this, SLOT(requestComplete(QString)));
	disconnect(this, SIGNAL(abortConnection()), connector, SLOT(cancelRequest()));
	qDebug() << "GAPI: Request Complete: " << response;
	emit complete(response);
}

QString GoogleMusicApi::fixLocalName(QString name) {
	if (name.contains("/")) {
		name = name.replace("/", "-");
	}
	if (name.contains("?")) {
		name = name.remove("?");
	}
	if (name.contains("...")) {
		name = name.remove("...");
	}
	return name;
}

void GoogleMusicApi::basicComplete(QString response) {
	// if success then save
	/*GoDataSource *data = new GoDataSource();
	data->setConnectionName("api");
	songToSync->insert("metaSynced", 1);
	data->updateSong(*songToSync);
	data->deleteLater();
	emit complete();*/
}

void GoogleMusicApi::albumArtDataComplete(QByteArray data) {
	//qDebug() << "GMA: Album Art complete";
	connector->deleteLater();
	emit albumArtData(data);
	emit complete();
}

void GoogleMusicApi::cancelDownloads() {
	qDebug() << "GMA: Aborting connections";
	emit abortConnection();
}

GoogleMusicApi::~GoogleMusicApi() {
}



