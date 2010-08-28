#include <QtCore>
#include <QtGui>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "qumdmodel.h" //to be removed

class sqldata
{
public:
    QString   absoluteFilePath;
    QString   filename;
    QDateTime lastModified;
    u32       lastModinSec;
    qint64    filesize;
	

   sqldata(QFileInfo const &entry)
   {
       read_cache(entry);//first read the cache if we already have an entry
   }
   void read_cache(QFileInfo const &entry)
   {
      QSqlQuery query;

      absoluteFilePath = entry.absoluteFilePath();
      filename         = entry.fileName();
      lastModified     = entry.lastModified();
      lastModinSec     = lastModified.toTime_t();
      filesize         = entry.size();
      //read the cache table for any entries based on filepath
      query.exec("SELECT * FROM cache where path = '" + absoluteFilePath + "'");
      if(query.first())
	  {
	     //get entries from database
	     u32 lastmodif_database = query.value(2).toUInt();
		 quint64 filesize_database = query.value(3).toULongLong();
		 //check if the entry that we have on cache is the same file by comparing filesize and lastmodinsec
	     bool lastmodsame  = (lastModinSec == lastmodif_database);
         bool filesizesame = (filesize == filesize_database);
         if (lastmodsame && filesizesame)
         {
             //cache entry is correct set available field to 1
	         query.exec("UPDATE cache SET available = 1 WHERE path = '" + absoluteFilePath + "'");
			 return;
		 }
    
	  }
      //not entry found
	   int     f;
       u8     *data;
       u32     size;
	   QString discid;
	   u32     crc32;
	   QString   status;
       u32 tables_relation=0;
	   umdimageloader::reboot(absoluteFilePath.toLatin1().data()); // it sucks ! it cannot handle LATIN1 or UTF8 names !!!

       size = ISOFS_getfilesize("PSP_GAME/PARAM.SFO");
       data = new u8[size];
       f = ISOFS_open("PSP_GAME/PARAM.SFO", 1);
       ISOFS_read(f, (char *)data, size);
       ISOFS_close(f);

       if (size)
       {
           psf::load_psf(data);
		   //look if there is an entry of it on games table
		   discid = QString::fromUtf8(psfinfo.disc_id).trimmed(); 
		   tables_relation = read_games_table(discid);
		   if(tables_relation==0)//if  0 then we haven't found a number
		   {
             //TODO write_games_table
		   }
		   //calculate crc
		   crc32 = Crc32().FullCRC(entry);
           //read icon
		   QDir().mkpath("data/" + discid);
           QPixmap icon0File(144, 80);
           QString icon0FileName((discid.size() ? ("data/" + discid) : ":/images") + "/icon0.png");
           if (QFile::exists(icon0FileName))
           {
              icon0File.load(icon0FileName);
            }
            else
            {
                size = ISOFS_getfilesize("PSP_GAME/ICON0.PNG");
                if (size)
                {
                   data = new u8[size];
                   f = ISOFS_open("PSP_GAME/ICON0.PNG", 1);
                   ISOFS_read(f, (char *)data, size);
                   ISOFS_close(f);
                   icon0File.loadFromData(data, size);
                }
                else
                {
                   data = 0;
                   icon0File.load(":/images/icon0.png");
                }
                   icon0File.save(icon0FileName);
                    delete data;
             }
		     int header;
		     status.clear();

             size = ISOFS_getfilesize("PSP_GAME/SYSDIR/BOOT.BIN");
             if (size)
             {
                int header;
                f = ISOFS_open("PSP_GAME/SYSDIR/BOOT.BIN", 1);
                ISOFS_read(f, (char *)&header, sizeof(int));
                ISOFS_close(f);

                switch (header)
                {
                   case 0x464C457F: status = "Plain BOOT.BIN"; break;
                   case 0x5053507E: status = "Encrypted BOOT.BIN (PSP~)"; break;
                   case 0x4543537E: status = "Encrypted BOOT.BIN (~SCE)"; break;
                   case 0x00000000: break;
                   default:         status = "Unsupported BOOT.BIN"; break;
                 }
               }

               if (!status.size())
               {
                        size = ISOFS_getfilesize("PSP_GAME/SYSDIR/EBOOT.BIN");
                        f = ISOFS_open("PSP_GAME/SYSDIR/EBOOT.BIN", 1);
                        ISOFS_read(f, (char *)&header, sizeof(int));
                        ISOFS_close(f);

                        switch (header)
                        {
                        case 0x464C457F: status = "Plain EBOOT.BIN"; break;
                        case 0x5053507E: status = "Encrypted EBOOT.BIN (PSP~)"; break;
                        case 0x4543537E: status = "Encrypted EBOOT.BIN (~SCE)"; break;
                        default:         status = "Unsupported EBOOT.BIN"; break;
                        }
               }
               //create a cache entry
			  query.prepare("INSERT INTO cache (path,lastmodified,filesize,gameid,crc32,gamestatus,available) "
              "VALUES (?,?,?,?,?,?,?)");
			  query.addBindValue(absoluteFilePath);
			  query.addBindValue(lastModinSec);
			  query.addBindValue(filesize);
			  query.addBindValue(tables_relation);
			  query.addBindValue(QString("%1").arg(crc32, 8, 16, QLatin1Char('0 ')).toUpper());
			  query.addBindValue(status);
			  query.addBindValue(1);//since we add a record it is added as available
			  query.exec();
			  //query.exec("UPDATE cache SET available = 1 WHERE path = '" + absoluteFilePath + "'");
	   }

   }
   u32 read_games_table(QString discid)
   {
      QSqlQuery query;
	  query.exec("SELECT * FROM games where discid = '" + discid + "'");
      if(query.first())//entry exists
	  {
        return query.value(0).toUInt();
	  }
	  return 0;//entry not found
   }
};