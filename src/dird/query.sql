:List Job totals:
SELECT count(*) AS Jobs,sum(JobFiles) AS Files, 
  sum(JobBytes) AS Bytes,Name AS Job 
 FROM Job 
 GROUP BY Name;
SELECT max(JobId) AS Jobs,sum(JobFiles) AS Files,sum(JobBytes) As Bytes 
 FROM Job;
# 2 
:List up to 20 places where a File is saved regardless of the directory:
*Enter Filename (no path):
SELECT DISTINCT Job.JobId as JobId, Client.Name as Client,
  Path.Path,Filename.Name,StartTime,Level,JobFiles,JobBytes
 FROM Client,Job,File,Filename,Path WHERE Client.ClientId=Job.ClientId
 AND JobStatus='T' AND Job.JobId=File.JobId
 AND Path.PathId=File.PathId AND Filename.FilenameId=File.FilenameId
 AND Filename.Name='%1' 
 ORDER BY Job.StartTime LIMIT 20;
# 3
:List where the most recent copies of a file are saved:
*Enter path with trailing slash:
*Enter filename:
*Enter Client name:
SELECT DISTINCT Job.JobId,StartTime AS JobStartTime,VolumeName,Client.Name AS ClientName
 FROM Job,File,Path,Filename,Media,JobMedia,Client
 WHERE File.JobId=Job.JobId
 AND Path.Path='%1'
 AND Filename.Name='%2'
 AND Client.Name='%3'
 AND Path.PathId=File.PathId
 AND Filename.FilenameId=File.FilenameId
 AND JobMedia.JobId=Job.JobId
 AND JobMedia.MediaId=Media.MediaId
 AND Client.ClientId=Job.ClientId
 ORDER BY Job.StartTime DESC LIMIT 5;
# 4
:List last 20 Full Backups for a Client:
*Enter Client name:
SELECT DISTINCT Job.JobId,Client.Name AS Client,StartTime,JobFiles,JobBytes,
  JobMedia.StartFile as VolFile,VolumeName
 FROM Client,Job,JobMedia,Media
 WHERE Client.Name='%1'
 AND Client.ClientId=Job.ClientId
 AND Level='F' AND JobStatus='T'
 AND JobMedia.JobId=Job.JobId AND JobMedia.MediaId=Media.MediaId
 ORDER BY Job.StartTime DESC LIMIT 20;
# 5
:List all backups for a Client after a specified time
*Enter Client Name:
*Enter time in YYYY-MM-DD HH:MM:SS format:
SELECT DISTINCT Job.JobId,Client.Name as Client,Level,StartTime,JobFiles,JobBytes,VolumeName
 FROM Client,Job,JobMedia,Media
 WHERE Client.Name='%1'
 AND Client.ClientId=Job.ClientId
 AND JobStatus='T'
 AND JobMedia.JobId=Job.JobId AND JobMedia.MediaId=Media.MediaId
 AND Job.StartTime >= '%2'
 ORDER BY Job.StartTime;
# 6
:List all backups for a Client
*Enter Client Name:
SELECT DISTINCT Job.JobId as JobId,Client.Name as Client,
   FileSet.FileSet AS FileSet,Level,StartTime,
   JobFiles,JobBytes,VolumeName
 FROM Client,Job,JobMedia,Media,FileSet
 WHERE Client.Name='%1'
 AND Client.ClientId=Job.ClientId
 AND JobStatus='T' AND Job.FileSetId=FileSet.FileSetId
 AND JobMedia.JobId=Job.JobId AND JobMedia.MediaId=Media.MediaId
 ORDER BY Job.StartTime;
# 7
:List Volume Attributes for a selected Volume:
*Enter Volume name:
SELECT Slot,MaxVolBytes,VolCapacityBytes,VolStatus,Recycle,VolRetention,
  VolUseDuration,MaxVolJobs,MaxVolFiles
 FROM Media   
 WHERE VolumeName='%1';
# 8
:List Volumes used by selected JobId:
*Enter JobId:
SELECT DISTINCT Job.JobId,VolumeName 
 FROM Job,JobMedia,Media 
 WHERE Job.JobId=%1 
 AND Job.JobId=JobMedia.JobId 
 AND JobMedia.MediaId=Media.MediaId;
# 9
:List Volumes to Restore All Files:
*Enter Client Name:
!DROP TABLE temp;
!DROP TABLE temp2;
CREATE TABLE temp (JobId BIGINT NOT NULL,
 JobTDate BIGINT,
 ClientId BIGINT,
 Level CHAR,
 StartTime TEXT,
 VolumeName TEXT,
 StartFile BIGINT, 
 VolSessionId BIGINT,
 VolSessionTime BIGINT );
CREATE TABLE temp2 (JobId BIGINT NOT NULL,
 StartTime TEXT,
 VolumeName TEXT,
 Level CHAR,
 StartFile BIGINT, 
 VolSessionId BIGINT,
 VolSessionTime BIGINT);
# Select last Full save
INSERT INTO temp SELECT Job.JobId,JobTDate,Job.ClientId,Job.Level,
  StartTime,VolumeName,JobMedia.StartFile,VolSessionId,VolSessionTime
 FROM Client,Job,JobMedia,Media WHERE Client.Name='%1'
 AND Client.ClientId=Job.ClientId
 AND Level='F' AND JobStatus='T'
 AND JobMedia.JobId=Job.JobId 
 AND JobMedia.MediaId=Media.MediaId
 ORDER BY Job.JobTDate DESC LIMIT 1;
# Copy into temp 2 getting all volumes of Full save
INSERT INTO temp2 SELECT Job.JobId,Job.StartTime,Media.VolumeName,Job.Level,
  JobMedia.StartFile,Job.VolSessionId,Job.VolSessionTime
 FROM temp,Job,JobMedia,Media WHERE temp.JobId=Job.JobId
 AND Job.Level='F' AND Job.JobStatus='T'
 AND JobMedia.JobId=Job.JobId
 AND JobMedia.MediaId=Media.MediaId;
# Now add subsequent incrementals
INSERT INTO temp2 SELECT DISTINCT Job.JobId,Job.StartTime,Media.VolumeName,
  Job.Level,JobMedia.StartFile,Job.VolSessionId,Job.VolSessionTime
 FROM Job,temp,JobMedia,Media
 WHERE Job.JobTDate>temp.JobTDate 
 AND Job.ClientId=temp.ClientId
 AND Job.Level IN ('I','D') AND JobStatus='T'
 AND JobMedia.JobId=Job.JobId 
 AND JobMedia.MediaId=Media.MediaId;
# list results
SELECT DISTINCT VolumeName from temp2;
!DROP TABLE temp;
!DROP TABLE temp2;
# 10
:List Pool Attributes for a selected Pool:
*Enter Pool name:
SELECT Recycle,VolRetention,VolUseDuration,MaxVolJobs,MaxVolFiles,MaxVolBytes
 FROM Pool
 WHERE Name='%1';
# 11
:List total files/bytes by Job:
SELECT count(*) AS Jobs,sum(JobFiles) AS Files,sum(JobBytes) AS Bytes,Name AS Job
 FROM Job GROUP by Name;
# 12
:List total files/bytes by Volume:
SELECT count(*) AS Jobs,sum(JobFiles) AS Files,sum(JobBytes) AS Bytes,VolumeName
 FROM Job,JobMedia,Media
 WHERE JobMedia.JobId=Job.JobId
 AND JobMedia.MediaId=Media.MediaId
 GROUP by VolumeName;  
# 13
:List Files for a selected JobId:
*Enter JobId:
SELECT Path.Path,Filename.Name FROM File,Filename,Path WHERE File.JobId=%1 
 AND Filename.FilenameId=File.FilenameId 
 AND Path.PathId=File.PathId ORDER BY
 Path.Path,Filename.Name;
# 14
:List Jobs stored in a selected MediaId:
*Enter MediaId:
SELECT DISTINCT Job.JobId,Job.Name,Job.StartTime,Job.Type,
  Job.Level,Job.JobFiles,Job.JobBytes,Job.JobStatus
 FROM JobMedia,Job
 WHERE JobMedia.JobId=Job.JobId
 AND JobMedia.MediaId=%1 
 ORDER by Job.StartTime;
# 15  
:List Jobs stored for a given Volume name:
*Enter Volume name:
SELECT DISTINCT Job.JobId as JobId,Job.Name as Name,Job.StartTime as StartTime,
  Job.Type as Type,Job.Level as Level,Job.JobFiles as Files,
  Job.JobBytes as Bytes,Job.JobStatus as Status
 FROM Media,JobMedia,Job
 WHERE Media.VolumeName='%1'
 AND Media.MediaId=JobMedia.MediaId              
 AND JobMedia.JobId=Job.JobId
 ORDER by Job.StartTime;
# 16
:List Volumes Bacula thinks are in changer:
SELECT MediaId,VolumeName,VolBytes/(1024*1024*1024) AS GB,Storage.Name 
  AS Storage,Slot,Pool.Name AS Pool,MediaType,VolStatus
  FROM Media,Pool,Storage
  WHERE Media.PoolId=Pool.PoolId
  AND Slot>0 AND InChanger=1
  AND Media.StorageId=Storage.StorageId
  ORDER BY MediaType ASC, Slot ASC;
# 17
:List Volumes likely to need replacement from age or errors
SELECT VolumeName AS Volume,VolMounts AS Mounts,VolErrors AS Errors,
         VolWrites AS Writes,VolStatus AS Status
  FROM Media
  WHERE (VolErrors>0) OR (VolStatus='Error') OR (VolMounts>50) OR
         (VolStatus='Disabled') OR (VolWrites>3999999)
  ORDER BY VolStatus ASC, VolErrors,VolMounts,VolumeName DESC;