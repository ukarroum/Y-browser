#include <stdexcept>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <format>
#include <iostream>
#include <sstream>
#include <regex>
#include <unordered_map>
#include <algorithm>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>

#include "URL.hpp"

URL::URL(const std::string &url)
{
    size_t split_pos = url.find("://");
    if(split_pos == std::string::npos)
        throw std::invalid_argument("Url invalid couldn't find '://'");

    m_scheme = url.substr(0, split_pos);
    auto host_path = url.substr(split_pos + 3);

    // Only http is supported for now
    // Using a string for this, sounds a little wasteful
    if(m_scheme != "http")
        throw std::invalid_argument("Only http is supported for now");

    split_pos = host_path.find('/');

    if(split_pos == std::string::npos)
    {
        m_host = host_path;
        m_path = "/";
    }
    else
    {
        m_host = host_path.substr(0, split_pos);
        m_path = host_path.substr(split_pos);
    }
}

std::string URL::request() const
{
    // The socket connection part if copied from Beej's guide to Network programming
    // I think it will be a better idea to have it in a separate class to hide it and use RAII
    int sockfd, rv;
    long numbytes;
    addrinfo hints{.ai_family = AF_INET, .ai_socktype = SOCK_STREAM, .ai_protocol = IPPROTO_TCP};
    addrinfo *servinfo, *p;
    char s[INET6_ADDRSTRLEN];
    char buf[maxdatasize];

    std::string response;

    // The port is hardcoded for now
    if(rv = getaddrinfo(m_host.c_str(), "80", &hints, &servinfo); rv != 0)
        throw std::runtime_error(std::format("Error using getaddrinfo: {}", gai_strerror(rv)));

    for(p = servinfo; p ; p = p->ai_next)
    {
        if (sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol); sockfd == -1)
        {
            std::cerr << "client: connect" << std::endl;
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            std::cerr << "client: connect" << std::endl;
            continue;
        }

        break;
    }

    if(!p)
        throw std::runtime_error("failed to connect");

    inet_ntop(p->ai_family, &(((struct sockaddr_in*)p->ai_addr)->sin_addr), s, sizeof(s));
    freeaddrinfo(servinfo);

    std::cout << "Connecting to: " << s << std::endl;

    // Constructing the request
    std::string request_s = std::format(
            "GET {} HTTP/1.0\r\n"
            "HOST: {}\r\n",
            m_path,
            m_host);

    for(auto& compress: available_decompressions)
        request_s += "Accept-Encoding: " + compress.first + "\r\n";

    request_s += "\r\n";

    if(numbytes = send(sockfd, request_s.c_str(), request_s.size(), 0); numbytes == -1)
        throw std::runtime_error("send");

    do
    {
        if(numbytes = recv(sockfd, buf, maxdatasize - 1, 0); numbytes == -1)
            throw std::runtime_error("recv");

        buf[numbytes] = '\0';

        response += buf;
    }while(numbytes > 0);

    response = std::regex_replace(response, std::regex("\r\n"), "\n");

    auto ss = std::stringstream{response};
    std::string line;

    // HTTP/1.0 200 OK
    getline(ss, line);
    std::string version{line.substr(0, line.find(' '))};
    line = line.substr(line.find(' ') + 1);
    std::string status{line.substr(0, line.find(' '))};
    std::string explanation{line.substr(line.find(' ') + 1)};

    // headers:
    // Accept-Ranges: bytes ...
    std::unordered_map<std::string, std::string> headers;

    while(getline(ss, line))
    {
        if(line.empty())
            break;
        // The +2 here is to skip both ":" and the space after
        // Header key is case-insensitive
        std::transform(line.begin(), line.begin() + line.find(':'), line.begin(), [](auto c) { return std::tolower(c); });

        headers[line.substr(0, line.find(':'))] = line.substr(line.find(':') + 2);
    }
    // html body
    std::string body;
    while(getline(ss, line))
    {
        body += line;
    }

    if(headers.contains("transfer-encoding"))
        throw std::runtime_error("transfer-encoding not handled");
    if(headers.contains("content-encoding"))
    {
        if(!available_decompressions.contains(headers["content-encoding"]))
            throw std::runtime_error(std::format("Encoding {} is not handled", headers["content-encoding"]));

        body = available_decompressions.at(headers["content-encoding"])(body);
    }

    close(sockfd);

    return body;
}

std::string URL::gzip_decompress(const std::string& compressed_text)
{
    std::istringstream compressed{compressed_text};
    std::ostringstream clear{};

    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(boost::iostreams::gzip_decompressor());
    in.push(compressed);
    boost::iostreams::copy(in, clear);

    return clear.str();
}
